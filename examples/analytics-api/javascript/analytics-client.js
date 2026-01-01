/**
 * TESAIoT BDH AI Analytics API Client
 *
 * A JavaScript client library for accessing the TESAIoT Analytics API.
 *
 * Usage:
 *   import { AnalyticsClient } from './analytics-client.js';
 *
 *   const client = new AnalyticsClient({
 *       baseUrl: 'https://admin.tesaiot.com/api/v1/bdh-ai',
 *       apiToken: 'your_jwt_token'
 *   });
 *
 *   const anomalies = await client.getAnomalies();
 *
 * Author: TESAIoT Team
 * License: Apache 2.0
 */

// ============================================================
// Time Range Helper
// ============================================================

export class TimeRange {
    constructor(start, end) {
        this.start = start;
        this.end = end;
    }

    /**
     * Create time range for last N days
     * @param {number} days - Number of days
     * @returns {TimeRange}
     */
    static lastDays(days) {
        const end = new Date();
        const start = new Date(end.getTime() - days * 24 * 60 * 60 * 1000);
        return new TimeRange(
            start.toISOString(),
            end.toISOString()
        );
    }

    /**
     * Create time range for last N hours
     * @param {number} hours - Number of hours
     * @returns {TimeRange}
     */
    static lastHours(hours) {
        const end = new Date();
        const start = new Date(end.getTime() - hours * 60 * 60 * 1000);
        return new TimeRange(
            start.toISOString(),
            end.toISOString()
        );
    }
}

// ============================================================
// Analytics Client
// ============================================================

export class AnalyticsClient {
    /**
     * Initialize the Analytics client
     * @param {Object} options - Client options
     * @param {string} [options.baseUrl] - API base URL
     * @param {string} [options.apiToken] - JWT authentication token
     * @param {number} [options.timeout] - Request timeout in ms (default: 30000)
     */
    constructor(options = {}) {
        this.baseUrl = options.baseUrl ||
            process.env.TESAIOT_API_URL ||
            'https://admin.tesaiot.com/api/v1/bdh-ai';

        this.apiToken = options.apiToken || process.env.TESAIOT_API_TOKEN;
        this.timeout = options.timeout || 30000;

        if (!this.apiToken) {
            throw new Error('API token required. Set TESAIOT_API_TOKEN or pass apiToken option');
        }
    }

    /**
     * Make an API request
     * @private
     */
    async _request(method, path, body = null, params = null) {
        let url = `${this.baseUrl}${path}`;

        if (params) {
            const searchParams = new URLSearchParams();
            for (const [key, value] of Object.entries(params)) {
                if (value !== null && value !== undefined) {
                    searchParams.append(key, value);
                }
            }
            const queryString = searchParams.toString();
            if (queryString) {
                url += `?${queryString}`;
            }
        }

        const options = {
            method,
            headers: {
                'X-API-KEY': this.apiToken,
                'Content-Type': 'application/json'
            },
            signal: AbortSignal.timeout(this.timeout)
        };

        if (body) {
            options.body = JSON.stringify(body);
        }

        const response = await fetch(url, options);

        if (!response.ok) {
            const error = await response.text();
            throw new Error(`API Error ${response.status}: ${error}`);
        }

        return response.json();
    }

    // --------------------------------------------------------
    // Anomaly Detection APIs
    // --------------------------------------------------------

    /**
     * Get aggregated anomalies
     * @param {Object} options - Query options
     * @param {string} [options.startTime] - Start of time range (ISO 8601)
     * @param {string} [options.endTime] - End of time range (ISO 8601)
     * @param {string[]} [options.severityFilter] - Filter by severity levels
     * @param {string[]} [options.deviceIds] - Filter by specific devices
     * @param {number} [options.limit=100] - Maximum results
     * @param {number} [options.offset=0] - Pagination offset
     * @returns {Promise<Object>}
     */
    async getAnomalies(options = {}) {
        // Use GET method with query parameters (API uses GET, not POST)
        const params = {
            limit: options.limit || 100,
            offset: options.offset || 0
        };

        if (options.severityFilter && options.severityFilter.length > 0) {
            params.severity = options.severityFilter.join(',');
        }
        if (options.deviceIds && options.deviceIds.length > 0) {
            params.device_ids = options.deviceIds.join(',');
        }

        return this._request('GET', '/anomalies', null, params);
    }

    /**
     * Get anomaly timeline for trend visualization
     * @param {Object} options - Query options
     * @param {number} [options.days=30] - Number of days to include
     * @param {string} [options.groupBy='severity'] - Grouping field
     * @returns {Promise<Object>}
     */
    async getAnomalyTimeline(options = {}) {
        return this._request('POST', '/analytics/anomalies/timeline', {
            days: options.days || 30,
            group_by: options.groupBy || 'severity'
        });
    }

    /**
     * Get device x time heatmap data
     * @param {Object} options - Query options
     * @param {string} options.startTime - Start of time range
     * @param {string} options.endTime - End of time range
     * @param {string} [options.resolution='hour'] - Time resolution
     * @param {number} [options.maxDevices=50] - Max devices to include
     * @returns {Promise<Object>}
     */
    async getAnomalyHeatmap(options) {
        return this._request('POST', '/analytics/anomalies/heatmap', {
            time_range: { start: options.startTime, end: options.endTime },
            resolution: options.resolution || 'hour',
            max_devices: options.maxDevices || 50
        });
    }

    /**
     * Acknowledge an anomaly
     * @param {string} anomalyId - The anomaly ID
     * @param {string} [notes] - Optional notes
     * @returns {Promise<Object>}
     */
    async acknowledgeAnomaly(anomalyId, notes = null) {
        const payload = { acknowledged: true };
        if (notes) payload.notes = notes;
        return this._request('PUT', `/analytics/anomalies/${anomalyId}/acknowledge`, payload);
    }

    /**
     * Resolve an anomaly
     * @param {string} anomalyId - The anomaly ID
     * @param {string} [resolution='resolved'] - Resolution type
     * @param {string} [notes] - Optional notes
     * @returns {Promise<Object>}
     */
    async resolveAnomaly(anomalyId, resolution = 'resolved', notes = null) {
        const payload = { resolved: true, resolution };
        if (notes) payload.notes = notes;
        return this._request('PUT', `/analytics/anomalies/${anomalyId}/resolve`, payload);
    }

    // --------------------------------------------------------
    // Pattern Recognition APIs
    // --------------------------------------------------------

    /**
     * Get K-means clustering of device behavior patterns
     * @param {Object} options - Query options
     * @param {string} options.metricName - Metric to cluster
     * @param {number} [options.nClusters=5] - Number of clusters
     * @param {string} [options.startTime] - Start of analysis period
     * @param {string} [options.endTime] - End of analysis period
     * @param {boolean} [options.includeOutliers=true] - Include outlier detection
     * @returns {Promise<Object>}
     */
    async getClusters(options) {
        const timeRange = options.startTime && options.endTime
            ? { start: options.startTime, end: options.endTime }
            : TimeRange.lastDays(7);

        return this._request('POST', '/patterns/clusters', {
            metric_name: options.metricName,
            n_clusters: options.nClusters || 5,
            time_range: { start: timeRange.start, end: timeRange.end },
            include_outliers: options.includeOutliers !== false
        });
    }

    /**
     * Get outlier devices
     * @param {Object} options - Query options
     * @param {string} options.metricName - Metric to analyze
     * @param {number} [options.threshold=0.7] - Outlier score threshold
     * @returns {Promise<Object>}
     */
    async getOutliers(options) {
        return this._request('POST', '/patterns/outliers', {
            metric_name: options.metricName,
            threshold: options.threshold || 0.7
        });
    }

    /**
     * Find devices with similar behavior patterns
     * @param {Object} options - Query options
     * @param {string} options.deviceId - Source device ID
     * @param {string} options.metricName - Metric to compare
     * @param {number} [options.topK=10] - Number of similar devices
     * @returns {Promise<Object>}
     */
    async findSimilarDevices(options) {
        return this._request('POST', '/patterns/similar', {
            device_id: options.deviceId,
            metric_name: options.metricName,
            top_k: options.topK || 10
        });
    }

    // --------------------------------------------------------
    // Insights APIs
    // --------------------------------------------------------

    /**
     * Generate AI-powered insights
     * @param {Object} options - Query options
     * @param {number} [options.days=7] - Analysis period in days
     * @param {string[]} [options.insightTypes] - Filter by type
     * @param {number} [options.minConfidence=0.7] - Minimum confidence score
     * @returns {Promise<Object>}
     */
    async getInsights(options = {}) {
        const payload = {
            analysis_period_days: options.days || 7,
            min_confidence: options.minConfidence || 0.7
        };

        if (options.insightTypes) {
            payload.insight_types = options.insightTypes;
        }

        return this._request('POST', '/insights', payload);
    }

    /**
     * Get actionable recommendations
     * @param {Object} options - Query options
     * @param {string} [options.priority] - Filter by priority
     * @param {number} [options.limit=10] - Maximum recommendations
     * @returns {Promise<Object>}
     */
    async getRecommendations(options = {}) {
        return this._request('GET', '/insights/recommendations', null, {
            priority: options.priority,
            limit: options.limit || 10
        });
    }

    // --------------------------------------------------------
    // Connectivity APIs
    // --------------------------------------------------------

    /**
     * Get real-time device connectivity status
     * @param {Object} [options] - Query options
     * @param {string} [options.statusFilter] - Filter by status (online/offline)
     * @returns {Promise<Object>}
     */
    async getConnectivityStatus(options = {}) {
        return this._request('GET', '/connectivity/status', null, {
            status: options.statusFilter
        });
    }

    /**
     * Get network latency statistics
     * @param {Object} [options] - Query options
     * @param {number} [options.hours=24] - Time range in hours
     * @returns {Promise<Object>}
     */
    async getLatencyStats(options = {}) {
        return this._request('GET', '/connectivity/latency', null, {
            hours: options.hours || 24
        });
    }

    /**
     * Get message throughput statistics
     * @param {Object} [options] - Query options
     * @param {number} [options.hours=24] - Time range in hours
     * @returns {Promise<Object>}
     */
    async getThroughputStats(options = {}) {
        return this._request('GET', '/connectivity/throughput', null, {
            hours: options.hours || 24
        });
    }

    /**
     * Get connection quality scores for all devices
     * @returns {Promise<Object>}
     */
    async getConnectionQuality() {
        return this._request('GET', '/connectivity/quality');
    }
}

// ============================================================
// Export for CommonJS compatibility
// ============================================================

export default AnalyticsClient;
