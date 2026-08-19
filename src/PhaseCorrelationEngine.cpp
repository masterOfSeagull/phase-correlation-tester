#include "PhaseCorrelationEngine.h"

#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QImageReader>
#include <QStandardPaths>
#include <QVariantMap>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr float kMagnitudeEpsilon = 1.0e-12f;
}

PhaseCorrelationEngine::PhaseCorrelationEngine(QObject *parent)
    : QObject(parent)
{
}

bool PhaseCorrelationEngine::setImageA(const QUrl &url)
{
    cv::Mat probe;
    QString error;
    if (!loadGrayFloat(url, probe, error)) {
        setStatus(error);
        return false;
    }

    m_imageAUrl = url.toString();
    clearResult();
    emit imageAChanged();
    setStatus(QStringLiteral("Image A loaded: %1 x %2").arg(probe.cols).arg(probe.rows));
    return true;
}

bool PhaseCorrelationEngine::setImageB(const QUrl &url)
{
    cv::Mat probe;
    QString error;
    if (!loadGrayFloat(url, probe, error)) {
        setStatus(error);
        return false;
    }

    m_imageBUrl = url.toString();
    clearResult();
    emit imageBChanged();
    setStatus(QStringLiteral("Image B loaded: %1 x %2").arg(probe.cols).arg(probe.rows));
    return true;
}

void PhaseCorrelationEngine::analyze()
{
    if (m_imageAUrl.isEmpty() || m_imageBUrl.isEmpty()) {
        setStatus(QStringLiteral("Load both Image A and Image B first."));
        return;
    }

    cv::Mat a;
    cv::Mat b;
    QString error;
    if (!loadGrayFloat(QUrl(m_imageAUrl), a, error)) {
        setStatus(error);
        return;
    }
    if (!loadGrayFloat(QUrl(m_imageBUrl), b, error)) {
        setStatus(error);
        return;
    }

    if (a.size() != b.size()) {
        setStatus(QStringLiteral("Images must have identical dimensions. A is %1x%2; B is %3x%4.")
                      .arg(a.cols).arg(a.rows).arg(b.cols).arg(b.rows));
        clearResult();
        return;
    }

    // Quadrant swapping is simplest and unambiguous for even dimensions.
    // Losing at most one row/column does not alter the diagnostic purpose of the tool.
    const int fftWidth = a.cols & ~1;
    const int fftHeight = a.rows & ~1;
    if (fftWidth < 2 || fftHeight < 2) {
        setStatus(QStringLiteral("Images are too small for phase correlation."));
        clearResult();
        return;
    }
    if (fftWidth != a.cols || fftHeight != a.rows) {
        a = a(cv::Rect(0, 0, fftWidth, fftHeight)).clone();
        b = b(cv::Rect(0, 0, fftWidth, fftHeight)).clone();
    }

    QElapsedTimer timer;
    timer.start();

    if (m_hannWindow) {
        cv::Mat window;
        cv::createHanningWindow(window, a.size(), CV_32F);
        cv::multiply(a, window, a);
        cv::multiply(b, window, b);
    }

    cv::Mat f1;
    cv::Mat f2;
    cv::dft(a, f1, cv::DFT_COMPLEX_OUTPUT);
    cv::dft(b, f2, cv::DFT_COMPLEX_OUTPUT);

    // Cross-power spectrum: F1 * conj(F2), then discard magnitude.
    cv::Mat crossPower;
    cv::mulSpectrums(f1, f2, crossPower, 0, true);

    std::vector<cv::Mat> channels;
    cv::split(crossPower, channels);
    cv::Mat magnitude;
    cv::magnitude(channels[0], channels[1], magnitude);
    magnitude += kMagnitudeEpsilon;
    cv::divide(channels[0], magnitude, channels[0]);
    cv::divide(channels[1], magnitude, channels[1]);
    cv::merge(channels, crossPower);

    cv::Mat correlation;
    cv::idft(crossPower, correlation, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);
    fftShift(correlation);

    const QList<Peak> detected = detectPeaks(correlation);
    if (detected.isEmpty()) {
        setStatus(QStringLiteral("No positive correlation peak was found in the selected search area."));
        clearResult();
        return;
    }

    QString heatmap;
    if (!writeHeatmap(correlation, detected, heatmap, error)) {
        setStatus(error);
        clearResult();
        return;
    }

    m_peaks.clear();
    for (int i = 0; i < detected.size(); ++i) {
        const Peak &peak = detected[i];
        QVariantMap item;
        item.insert(QStringLiteral("rank"), i + 1);
        item.insert(QStringLiteral("dx"), peak.dx);
        item.insert(QStringLiteral("dy"), peak.dy);
        item.insert(QStringLiteral("strength"), peak.value);
        item.insert(QStringLiteral("relative"), peak.relative);
        m_peaks.append(item);
    }

    m_runtimeMs = static_cast<double>(timer.nsecsElapsed()) / 1.0e6;
    m_heatmapUrl = heatmap;
    m_detectedPeaks = detected;
    m_selectedPeakIndex = 0;
    m_hasResult = true;

    QString previewError;
    if (!refreshSelectedPreview(previewError)) {
        m_previewUrl.clear();
        m_matchedPercent = 0.0;
    }

    emit resultChanged();
    emit previewChanged();

    const Peak &best = detected.front();
    QString status = QStringLiteral("Best alignment shift for Image B: dx=%1, dy=%2 px. %3x%4 analyzed in %5 ms.")
                         .arg(best.dx, 0, 'f', 2)
                         .arg(best.dy, 0, 'f', 2)
                         .arg(correlation.cols)
                         .arg(correlation.rows)
                         .arg(m_runtimeMs, 0, 'f', 2);
    if (!previewError.isEmpty()) {
        status += QStringLiteral(" Candidate preview unavailable: %1").arg(previewError);
    }
    setStatus(status);
}

void PhaseCorrelationEngine::selectPeak(int index)
{
    if (!m_hasResult || index < 0 || index >= m_detectedPeaks.size()) {
        setStatus(QStringLiteral("Select a valid detected peak after running Analyze."));
        return;
    }

    m_selectedPeakIndex = index;
    QString error;
    if (!refreshSelectedPreview(error)) {
        m_previewUrl.clear();
        m_matchedPercent = 0.0;
        emit previewChanged();
        setStatus(QStringLiteral("Could not render candidate preview: %1").arg(error));
        return;
    }

    emit previewChanged();

    const Peak &peak = m_detectedPeaks[index];
    setStatus(QStringLiteral("Peak #%1 selected: shift Image B by dx=%2, dy=%3 px. %4% of overlapping pixels pass the %5% similarity threshold.")
                  .arg(index + 1)
                  .arg(peak.dx, 0, 'f', 2)
                  .arg(peak.dy, 0, 'f', 2)
                  .arg(m_matchedPercent, 0, 'f', 1)
                  .arg(m_similarityThreshold * 100.0, 0, 'f', 0));
}

void PhaseCorrelationEngine::setHannWindow(bool value)
{
    if (m_hannWindow == value) return;
    m_hannWindow = value;
    emit settingsChanged();
}

void PhaseCorrelationEngine::setLimitSearch(bool value)
{
    if (m_limitSearch == value) return;
    m_limitSearch = value;
    emit settingsChanged();
}

void PhaseCorrelationEngine::setMaxDx(int value)
{
    value = std::clamp(value, 0, 16384);
    if (m_maxDx == value) return;
    m_maxDx = value;
    emit settingsChanged();
}

void PhaseCorrelationEngine::setMaxDy(int value)
{
    value = std::clamp(value, 0, 16384);
    if (m_maxDy == value) return;
    m_maxDy = value;
    emit settingsChanged();
}

void PhaseCorrelationEngine::setPeakCount(int value)
{
    value = std::clamp(value, 1, 20);
    if (m_peakCount == value) return;
    m_peakCount = value;
    emit settingsChanged();
}

void PhaseCorrelationEngine::setSuppressionRadius(int value)
{
    value = std::clamp(value, 1, 512);
    if (m_suppressionRadius == value) return;
    m_suppressionRadius = value;
    emit settingsChanged();
}

void PhaseCorrelationEngine::setSimilarityThreshold(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (std::abs(m_similarityThreshold - value) < 1.0e-9) return;

    m_similarityThreshold = value;
    emit settingsChanged();

    if (!m_hasResult || m_selectedPeakIndex < 0 || m_selectedPeakIndex >= m_detectedPeaks.size()) {
        return;
    }

    QString error;
    if (!refreshSelectedPreview(error)) {
        m_previewUrl.clear();
        m_matchedPercent = 0.0;
        emit previewChanged();
        setStatus(QStringLiteral("Could not update candidate preview: %1").arg(error));
        return;
    }

    emit previewChanged();
    const Peak &peak = m_detectedPeaks[m_selectedPeakIndex];
    setStatus(QStringLiteral("Peak #%1 preview updated: dx=%2, dy=%3 px; %4% of overlap passes the %5% similarity threshold.")
                  .arg(m_selectedPeakIndex + 1)
                  .arg(peak.dx, 0, 'f', 2)
                  .arg(peak.dy, 0, 'f', 2)
                  .arg(m_matchedPercent, 0, 'f', 1)
                  .arg(m_similarityThreshold * 100.0, 0, 'f', 0));
}

bool PhaseCorrelationEngine::loadGrayFloat(const QUrl &url, cv::Mat &out, QString &error)
{
    if (!url.isLocalFile()) {
        error = QStringLiteral("Only local image files are supported.");
        return false;
    }

    QImageReader reader(url.toLocalFile());
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        error = QStringLiteral("Could not load image: %1").arg(reader.errorString());
        return false;
    }

    image = image.convertToFormat(QImage::Format_Grayscale8);
    cv::Mat view(image.height(), image.width(), CV_8UC1,
                 image.bits(), static_cast<size_t>(image.bytesPerLine()));
    view.convertTo(out, CV_32F, 1.0 / 255.0);
    return true;
}

bool PhaseCorrelationEngine::loadRgba8(const QUrl &url, cv::Mat &out, QString &error)
{
    if (!url.isLocalFile()) {
        error = QStringLiteral("Only local image files are supported.");
        return false;
    }

    QImageReader reader(url.toLocalFile());
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        error = QStringLiteral("Could not load image: %1").arg(reader.errorString());
        return false;
    }

    image = image.convertToFormat(QImage::Format_RGBA8888);
    cv::Mat view(image.height(), image.width(), CV_8UC4,
                 image.bits(), static_cast<size_t>(image.bytesPerLine()));
    out = view.clone();
    return true;
}

void PhaseCorrelationEngine::fftShift(cv::Mat &matrix)
{
    const int cx = matrix.cols / 2;
    const int cy = matrix.rows / 2;

    cv::Mat q0(matrix, cv::Rect(0, 0, cx, cy));
    cv::Mat q1(matrix, cv::Rect(cx, 0, cx, cy));
    cv::Mat q2(matrix, cv::Rect(0, cy, cx, cy));
    cv::Mat q3(matrix, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);

    q1.copyTo(tmp);
    q2.copyTo(q1);
    tmp.copyTo(q2);
}

double PhaseCorrelationEngine::subpixelOffset(double left, double center, double right)
{
    const double denominator = left - 2.0 * center + right;
    if (std::abs(denominator) < 1.0e-12) {
        return 0.0;
    }
    return std::clamp(0.5 * (left - right) / denominator, -0.5, 0.5);
}

QList<PhaseCorrelationEngine::Peak> PhaseCorrelationEngine::detectPeaks(const cv::Mat &correlation) const
{
    cv::Mat mask(correlation.size(), CV_8U, cv::Scalar(255));
    const int centerX = correlation.cols / 2;
    const int centerY = correlation.rows / 2;

    if (m_limitSearch) {
        mask.setTo(cv::Scalar(0));
        const int left = std::max(0, centerX - m_maxDx);
        const int right = std::min(correlation.cols - 1, centerX + m_maxDx);
        const int top = std::max(0, centerY - m_maxDy);
        const int bottom = std::min(correlation.rows - 1, centerY + m_maxDy);
        if (left <= right && top <= bottom) {
            mask(cv::Rect(left, top, right - left + 1, bottom - top + 1)).setTo(cv::Scalar(255));
        }
    }

    QList<Peak> result;
    double strongest = 0.0;

    for (int rank = 0; rank < m_peakCount; ++rank) {
        double maxValue = 0.0;
        cv::Point location;
        cv::minMaxLoc(correlation, nullptr, &maxValue, nullptr, &location, mask);
        if (!(maxValue > 0.0) || mask.at<uchar>(location) == 0) {
            break;
        }

        double subX = 0.0;
        double subY = 0.0;
        if (location.x > 0 && location.x + 1 < correlation.cols) {
            subX = subpixelOffset(
                correlation.at<float>(location.y, location.x - 1),
                correlation.at<float>(location.y, location.x),
                correlation.at<float>(location.y, location.x + 1));
        }
        if (location.y > 0 && location.y + 1 < correlation.rows) {
            subY = subpixelOffset(
                correlation.at<float>(location.y - 1, location.x),
                correlation.at<float>(location.y, location.x),
                correlation.at<float>(location.y + 1, location.x));
        }

        if (rank == 0) {
            strongest = maxValue;
        }

        Peak peak;
        peak.integerLocation = location;
        peak.dx = static_cast<double>(location.x - centerX) + subX;
        peak.dy = static_cast<double>(location.y - centerY) + subY;
        peak.value = maxValue;
        peak.relative = strongest > 0.0 ? maxValue / strongest : 0.0;
        result.append(peak);

        cv::circle(mask, location, m_suppressionRadius, cv::Scalar(0), cv::FILLED);
    }

    return result;
}

bool PhaseCorrelationEngine::writeHeatmap(const cv::Mat &correlation,
                                          const QList<Peak> &peaks,
                                          QString &outputUrl,
                                          QString &error)
{
    cv::Mat positive;
    cv::threshold(correlation, positive, 0.0, 0.0, cv::THRESH_TOZERO);

    double maxValue = 0.0;
    cv::minMaxLoc(positive, nullptr, &maxValue);
    if (!(maxValue > 0.0)) {
        error = QStringLiteral("Correlation surface contains no positive values to visualize.");
        return false;
    }

    positive /= maxValue;
    cv::sqrt(positive, positive); // gamma 0.5: weaker secondary peaks remain visible.

    cv::Mat gray8;
    positive.convertTo(gray8, CV_8U, 255.0);

    cv::Mat colored;
    cv::applyColorMap(gray8, colored, cv::COLORMAP_TURBO);

    const int centerX = correlation.cols / 2;
    const int centerY = correlation.rows / 2;
    cv::drawMarker(colored, cv::Point(centerX, centerY), cv::Scalar(255, 255, 255),
                   cv::MARKER_CROSS, 13, 1, cv::LINE_AA);

    if (m_limitSearch) {
        const int left = std::max(0, centerX - m_maxDx);
        const int right = std::min(correlation.cols - 1, centerX + m_maxDx);
        const int top = std::max(0, centerY - m_maxDy);
        const int bottom = std::min(correlation.rows - 1, centerY + m_maxDy);
        cv::rectangle(colored, cv::Rect(left, top, right - left + 1, bottom - top + 1),
                      cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }

    for (int i = 0; i < peaks.size(); ++i) {
        const cv::Point p = peaks[i].integerLocation;
        cv::circle(colored, p, 8, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        cv::putText(colored, std::to_string(i + 1), p + cv::Point(10, -8),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }

    cv::Mat rgb;
    cv::cvtColor(colored, rgb, cv::COLOR_BGR2RGB);
    QImage wrapped(rgb.data, rgb.cols, rgb.rows, static_cast<qsizetype>(rgb.step), QImage::Format_RGB888);
    QImage owned = wrapped.copy();

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDir.isEmpty()) {
        error = QStringLiteral("No writable temporary directory is available.");
        return false;
    }

    QDir().mkpath(tempDir);
    const QString path = QDir(tempDir).filePath(
        QStringLiteral("phase-correlation-heatmap-%1.png").arg(++m_revision));
    if (!owned.save(path, "PNG")) {
        error = QStringLiteral("Failed to save the correlation heatmap.");
        return false;
    }

    outputUrl = QUrl::fromLocalFile(path).toString();
    return true;
}

bool PhaseCorrelationEngine::writeCandidatePreview(const Peak &peak,
                                                   QString &outputUrl,
                                                   double &matchedPercent,
                                                   QString &error)
{
    cv::Mat a;
    cv::Mat b;
    if (!loadRgba8(QUrl(m_imageAUrl), a, error)) {
        return false;
    }
    if (!loadRgba8(QUrl(m_imageBUrl), b, error)) {
        return false;
    }
    if (a.size() != b.size()) {
        error = QStringLiteral("Images must have identical dimensions for candidate preview.");
        return false;
    }

    const int width = a.cols & ~1;
    const int height = a.rows & ~1;
    if (width < 2 || height < 2) {
        error = QStringLiteral("Images are too small for candidate preview.");
        return false;
    }
    if (width != a.cols || height != a.rows) {
        a = a(cv::Rect(0, 0, width, height)).clone();
        b = b(cv::Rect(0, 0, width, height)).clone();
    }

    // The detected phase-correlation displacement is the translation to apply to
    // Image B to bring that candidate into Image A's coordinate system.
    const cv::Mat transform = (cv::Mat_<double>(2, 3) <<
        1.0, 0.0, peak.dx,
        0.0, 1.0, peak.dy);

    cv::Mat alignedB;
    cv::warpAffine(b, alignedB, transform, a.size(), cv::INTER_LINEAR,
                   cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));

    cv::Mat sourceMask(b.rows, b.cols, CV_8U, cv::Scalar(255));
    cv::Mat overlapMask;
    cv::warpAffine(sourceMask, overlapMask, transform, a.size(), cv::INTER_NEAREST,
                   cv::BORDER_CONSTANT, cv::Scalar(0));

    cv::Mat preview(a.rows, a.cols, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    qint64 overlapCount = 0;
    qint64 matchedCount = 0;

    for (int y = 0; y < a.rows; ++y) {
        const cv::Vec4b *rowA = a.ptr<cv::Vec4b>(y);
        const cv::Vec4b *rowB = alignedB.ptr<cv::Vec4b>(y);
        const uchar *rowMask = overlapMask.ptr<uchar>(y);
        cv::Vec4b *rowOut = preview.ptr<cv::Vec4b>(y);

        for (int x = 0; x < a.cols; ++x) {
            if (rowMask[x] == 0 || rowA[x][3] == 0 || rowB[x][3] == 0) {
                continue;
            }

            ++overlapCount;
            const double rgbDifference = (
                std::abs(static_cast<int>(rowA[x][0]) - static_cast<int>(rowB[x][0])) +
                std::abs(static_cast<int>(rowA[x][1]) - static_cast<int>(rowB[x][1])) +
                std::abs(static_cast<int>(rowA[x][2]) - static_cast<int>(rowB[x][2]))) / (3.0 * 255.0);
            const double similarity = 1.0 - rgbDifference;

            if (similarity < m_similarityThreshold) {
                continue;
            }

            ++matchedCount;
            rowOut[x][0] = static_cast<uchar>((static_cast<int>(rowA[x][0]) + static_cast<int>(rowB[x][0]) + 1) / 2);
            rowOut[x][1] = static_cast<uchar>((static_cast<int>(rowA[x][1]) + static_cast<int>(rowB[x][1]) + 1) / 2);
            rowOut[x][2] = static_cast<uchar>((static_cast<int>(rowA[x][2]) + static_cast<int>(rowB[x][2]) + 1) / 2);
            rowOut[x][3] = 255;
        }
    }

    matchedPercent = overlapCount > 0
        ? 100.0 * static_cast<double>(matchedCount) / static_cast<double>(overlapCount)
        : 0.0;

    QImage wrapped(preview.data, preview.cols, preview.rows,
                   static_cast<qsizetype>(preview.step), QImage::Format_RGBA8888);
    QImage owned = wrapped.copy();

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDir.isEmpty()) {
        error = QStringLiteral("No writable temporary directory is available.");
        return false;
    }

    QDir().mkpath(tempDir);
    const QString path = QDir(tempDir).filePath(
        QStringLiteral("phase-correlation-candidate-%1.png").arg(++m_revision));
    if (!owned.save(path, "PNG")) {
        error = QStringLiteral("Failed to save the candidate alignment preview.");
        return false;
    }

    outputUrl = QUrl::fromLocalFile(path).toString();
    return true;
}

bool PhaseCorrelationEngine::refreshSelectedPreview(QString &error)
{
    if (m_selectedPeakIndex < 0 || m_selectedPeakIndex >= m_detectedPeaks.size()) {
        error = QStringLiteral("No peak candidate is selected.");
        return false;
    }

    QString outputUrl;
    double matchedPercent = 0.0;
    if (!writeCandidatePreview(m_detectedPeaks[m_selectedPeakIndex], outputUrl, matchedPercent, error)) {
        return false;
    }

    m_previewUrl = outputUrl;
    m_matchedPercent = matchedPercent;
    return true;
}

void PhaseCorrelationEngine::setStatus(const QString &message)
{
    if (m_statusMessage == message) return;
    m_statusMessage = message;
    emit statusChanged();
}

void PhaseCorrelationEngine::clearResult()
{
    const bool hadResultContent = m_hasResult || !m_heatmapUrl.isEmpty() || !m_peaks.isEmpty();
    const bool hadPreviewContent = !m_previewUrl.isEmpty() || m_selectedPeakIndex >= 0 || !m_detectedPeaks.isEmpty();

    m_heatmapUrl.clear();
    m_previewUrl.clear();
    m_peaks.clear();
    m_detectedPeaks.clear();
    m_runtimeMs = 0.0;
    m_matchedPercent = 0.0;
    m_hasResult = false;
    m_selectedPeakIndex = -1;

    if (hadResultContent) {
        emit resultChanged();
    }
    if (hadPreviewContent) {
        emit previewChanged();
    }
}
