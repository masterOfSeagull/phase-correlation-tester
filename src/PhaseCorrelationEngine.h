#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>

#include <opencv2/core.hpp>

class PhaseCorrelationEngine final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString imageAUrl READ imageAUrl NOTIFY imageAChanged)
    Q_PROPERTY(QString imageBUrl READ imageBUrl NOTIFY imageBChanged)
    Q_PROPERTY(QString heatmapUrl READ heatmapUrl NOTIFY resultChanged)
    Q_PROPERTY(QString previewUrl READ previewUrl NOTIFY previewChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(QVariantList peaks READ peaks NOTIFY resultChanged)
    Q_PROPERTY(double runtimeMs READ runtimeMs NOTIFY resultChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultChanged)
    Q_PROPERTY(int selectedPeakIndex READ selectedPeakIndex NOTIFY previewChanged)
    Q_PROPERTY(double matchedPercent READ matchedPercent NOTIFY previewChanged)

    Q_PROPERTY(bool hannWindow READ hannWindow WRITE setHannWindow NOTIFY settingsChanged)
    Q_PROPERTY(bool limitSearch READ limitSearch WRITE setLimitSearch NOTIFY settingsChanged)
    Q_PROPERTY(int maxDx READ maxDx WRITE setMaxDx NOTIFY settingsChanged)
    Q_PROPERTY(int maxDy READ maxDy WRITE setMaxDy NOTIFY settingsChanged)
    Q_PROPERTY(int peakCount READ peakCount WRITE setPeakCount NOTIFY settingsChanged)
    Q_PROPERTY(int suppressionRadius READ suppressionRadius WRITE setSuppressionRadius NOTIFY settingsChanged)
    Q_PROPERTY(double similarityThreshold READ similarityThreshold WRITE setSimilarityThreshold NOTIFY settingsChanged)
    Q_PROPERTY(double cropLeft READ cropLeft WRITE setCropLeft NOTIFY settingsChanged)
    Q_PROPERTY(double cropTop READ cropTop WRITE setCropTop NOTIFY settingsChanged)
    Q_PROPERTY(double cropRight READ cropRight WRITE setCropRight NOTIFY settingsChanged)
    Q_PROPERTY(double cropBottom READ cropBottom WRITE setCropBottom NOTIFY settingsChanged)

public:
    explicit PhaseCorrelationEngine(QObject *parent = nullptr);

    QString imageAUrl() const { return m_imageAUrl; }
    QString imageBUrl() const { return m_imageBUrl; }
    QString heatmapUrl() const { return m_heatmapUrl; }
    QString previewUrl() const { return m_previewUrl; }
    QString statusMessage() const { return m_statusMessage; }
    QVariantList peaks() const { return m_peaks; }
    double runtimeMs() const { return m_runtimeMs; }
    bool hasResult() const { return m_hasResult; }
    int selectedPeakIndex() const { return m_selectedPeakIndex; }
    double matchedPercent() const { return m_matchedPercent; }

    bool hannWindow() const { return m_hannWindow; }
    bool limitSearch() const { return m_limitSearch; }
    int maxDx() const { return m_maxDx; }
    int maxDy() const { return m_maxDy; }
    int peakCount() const { return m_peakCount; }
    int suppressionRadius() const { return m_suppressionRadius; }
    double similarityThreshold() const { return m_similarityThreshold; }
    double cropLeft() const { return m_cropLeft; }
    double cropTop() const { return m_cropTop; }
    double cropRight() const { return m_cropRight; }
    double cropBottom() const { return m_cropBottom; }

    Q_INVOKABLE bool setImageA(const QUrl &url);
    Q_INVOKABLE bool setImageB(const QUrl &url);
    Q_INVOKABLE void analyze();
    Q_INVOKABLE void selectPeak(int index);

    void setHannWindow(bool value);
    void setLimitSearch(bool value);
    void setMaxDx(int value);
    void setMaxDy(int value);
    void setPeakCount(int value);
    void setSuppressionRadius(int value);
    void setSimilarityThreshold(double value);
    void setCropLeft(double value);
    void setCropTop(double value);
    void setCropRight(double value);
    void setCropBottom(double value);

signals:
    void imageAChanged();
    void imageBChanged();
    void resultChanged();
    void previewChanged();
    void statusChanged();
    void settingsChanged();

private:
    struct Peak {
        double dx = 0.0;
        double dy = 0.0;
        double value = 0.0;
        double relative = 0.0;
        cv::Point integerLocation;
    };

    bool loadGrayFloat(const QUrl &url, cv::Mat &out, QString &error) const;
    bool loadRgba8(const QUrl &url, cv::Mat &out, QString &error) const;
    bool applyCrop(cv::Mat &matrix, QString &error) const;
    static void fftShift(cv::Mat &matrix);
    static double subpixelOffset(double left, double center, double right);

    QList<Peak> detectPeaks(const cv::Mat &correlation) const;
    bool writeHeatmap(const cv::Mat &correlation, const QList<Peak> &peaks, QString &outputUrl, QString &error);
    bool writeCandidatePreview(const Peak &peak, QString &outputUrl, double &matchedPercent, QString &error);
    bool refreshSelectedPreview(QString &error);
    void setStatus(const QString &message);
    void clearResult();

    QString m_imageAUrl;
    QString m_imageBUrl;
    QString m_heatmapUrl;
    QString m_previewUrl;
    QString m_statusMessage = QStringLiteral("Load two images to begin.");
    QVariantList m_peaks;
    QList<Peak> m_detectedPeaks;
    double m_runtimeMs = 0.0;
    double m_matchedPercent = 0.0;
    bool m_hasResult = false;
    int m_selectedPeakIndex = -1;

    bool m_hannWindow = true;
    bool m_limitSearch = false;
    int m_maxDx = 128;
    int m_maxDy = 128;
    int m_peakCount = 5;
    int m_suppressionRadius = 12;
    double m_similarityThreshold = 0.90;
    double m_cropLeft = 0.0;
    double m_cropTop = 0.0;
    double m_cropRight = 1.0;
    double m_cropBottom = 1.0;
    quint64 m_revision = 0;
};
