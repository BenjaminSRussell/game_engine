# AEGIS Rust Scraper Integration

## Overview

This document details the integration between the AEGIS Dashboard and the Rust-based web scraper. The integration provides real-time control, monitoring, and data visualization capabilities for the high-performance Rust scraping engine.

## Rust Scraper Parameters

### Core Configuration Parameters

| Parameter | Flag | Default | Description |
|-----------|------|---------|-------------|
| Target URL | `--start-url`, `-s` | Required | The starting URL for the crawl |
| Data Directory | `--data-dir`, `-d` | `./data` | Directory where logs and results are stored |
| Concurrency | `--workers`, `-w` | 2048 | Maximum number of concurrent requests |
| Timeout | `--timeout`, `-t` | 30 | Request timeout in seconds |
| User Agent | `--user-agent`, `-a` | Default String | User-Agent string sent with requests |
| Robots.txt | `--ignore-robots` | false | If set, ignores robots.txt rules |
| URL Limit | `--max-urls` | None | Stop after N URLs |
| Duration | `--duration` | None | Stop after N seconds |
| Proxy | `--proxy` | None | HTTP/HTTPS proxy URL |
| Seeding Strategy | `--seeding-strategy` | none | Sources: sitemap, ct, commoncrawl |

### Distributed Crawling (Redis)

| Parameter | Flag | Default | Description |
|-----------|------|---------|-------------|
| Enable Redis | `--enable-redis` | false | Use Redis for distributed state/frontier |
| Redis URL | `--redis-url` | localhost | Connection URL (e.g., redis://127.0.0.1:6379) |
| Lock TTL | `--lock-ttl` | 300 | Time-to-live for Redis locks in seconds |

### Preset Configurations

| Preset | Goal | Settings |
|--------|------|----------|
| `ben` / `max` | Maximum throughput | 1024 workers, 60s timeout, ignores robots.txt, Performance mode |
| `polite` | Low-load, respectful crawling | 32 workers, 10s timeout, respects robots.txt, Steady mode |
| `stealth` | Maximum evasion | 4 workers, 30s timeout, respects robots.txt, Stealth mode |

### Internal Crawl Modes

| Mode | Description |
|------|-------------|
| `Steady` | Balanced (25-75ms jitter, 100 connections) |
| `Stealth` | Evasive (50-300ms jitter, think time, 50 connections) |
| `Performance` | Raw speed (0ms jitter, 200 connections) |

## Integration Architecture

### Communication Protocol

```rust
// Rust scraper communication interface
pub struct ScraperControlInterface {
    command_sender: tokio::sync::mpsc::Sender<ScraperCommand>,
    status_receiver: tokio::sync::watch::Receiver<ScraperStatus>,
    metrics_receiver: tokio::sync::mpsc::Receiver<MetricUpdate>,
}

pub enum ScraperCommand {
    Start {
        config: ScraperConfig,
    },
    Pause {
        job_id: Uuid,
    },
    Resume {
        job_id: Uuid,
    },
    Stop {
        job_id: Uuid,
    },
    UpdateConfig {
        job_id: Uuid,
        config: ScraperConfig,
    },
    GetStatus {
        job_id: Uuid,
    },
}

pub struct ScraperStatus {
    pub job_id: Uuid,
    pub state: ScraperState,
    pub progress: ProgressInfo,
    pub statistics: Statistics,
    pub errors: Vec<ScraperError>,
}
```

### WebSocket Integration

```typescript
// Frontend WebSocket integration
export class ScraperWebSocketClient {
  private socket: WebSocket;
  private messageHandlers: Map<string, Function> = new Map();

  constructor(url: string) {
    this.socket = new WebSocket(url);
    this.setupEventHandlers();
  }

  private setupEventHandlers() {
    this.socket.onmessage = (event) => {
      const message = JSON.parse(event.data);
      const handler = this.messageHandlers.get(message.type);
      if (handler) {
        handler(message.data);
      }
    };
  }

  public subscribeToMetrics(jobId: string, callback: (metrics: Metric[]) => void) {
    this.messageHandlers.set('metrics', callback);
    this.send({
      type: 'subscribe_metrics',
      jobId: jobId,
    });
  }

  public subscribeToStatus(jobId: string, callback: (status: ScraperStatus) => void) {
    this.messageHandlers.set('status', callback);
    this.send({
      type: 'subscribe_status',
      jobId: jobId,
    });
  }

  public sendCommand(command: ScraperCommand) {
    this.send({
      type: 'command',
      command: command,
    });
  }

  private send(message: any) {
    if (this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message));
    }
  }
}
```

## Dashboard Controls Integration

### Concurrency Control

```typescript
// Concurrency slider integration
export const ConcurrencyControl: React.FC<{ jobId: string }> = ({ jobId }) => {
  const [concurrency, setConcurrency] = useState(2048);
  const [isUpdating, setIsUpdating] = useState(false);

  const handleConcurrencyChange = async (value: number) => {
    setIsUpdating(true);
    try {
      await scraperAPI.updateJobConfig(jobId, {
        workers: value,
      });
      setConcurrency(value);
      
      // Show success feedback
      toast.success(`Concurrency updated to ${value}`);
    } catch (error) {
      toast.error('Failed to update concurrency');
    } finally {
      setIsUpdating(false);
    }
  };

  return (
    <div className="concurrency-control">
      <label htmlFor="concurrency-slider">
        Max Concurrent Requests: {concurrency}
      </label>
      <Slider
        id="concurrency-slider"
        min={1}
        max={5000}
        step={1}
        value={concurrency}
        onChange={handleConcurrencyChange}
        disabled={isUpdating}
        className="w-full"
      />
      <div className="slider-labels">
        <span>1</span>
        <span>2500</span>
        <span>5000</span>
      </div>
    </div>
  );
};
```

### Timeout Control

```typescript
// Timeout control integration
export const TimeoutControl: React.FC<{ jobId: string }> = ({ jobId }) => {
  const [timeout, setTimeout] = useState(30);

  const handleTimeoutChange = async (value: number) => {
    try {
      await scraperAPI.updateJobConfig(jobId, {
        timeout: value,
      });
      setTimeout(value);
    } catch (error) {
      console.error('Failed to update timeout:', error);
    }
  };

  return (
    <div className="timeout-control">
      <label htmlFor="timeout-slider">
        Request Timeout: {timeout}s
      </label>
      <Slider
        id="timeout-slider"
        min={1}
        max={300}
        step={1}
        value={timeout}
        onChange={handleTimeoutChange}
        marks={[
          { value: 10, label: '10s' },
          { value: 30, label: '30s' },
          { value: 60, label: '1m' },
          { value: 120, label: '2m' },
        ]}
        className="w-full"
      />
    </div>
  );
};
```

### Preset Selection

```typescript
// Preset configuration selector
export const PresetSelector: React.FC<{ jobId: string }> = ({ jobId }) => {
  const presets = [
    { id: 'ben', name: 'Maximum Performance', description: 'High speed, no politeness' },
    { id: 'polite', name: 'Polite Crawling', description: 'Respectful, low impact' },
    { id: 'stealth', name: 'Stealth Mode', description: 'Maximum evasion techniques' },
  ];

  const [selectedPreset, setSelectedPreset] = useState<string | null>(null);

  const applyPreset = async (presetId: string) => {
    try {
      await scraperAPI.applyPreset(jobId, presetId);
      setSelectedPreset(presetId);
      toast.success(`Applied ${presetId} preset`);
    } catch (error) {
      toast.error('Failed to apply preset');
    }
  };

  return (
    <div className="preset-selector">
      <h3>Crawl Presets</h3>
      <div className="preset-grid">
        {presets.map((preset) => (
          <Card
            key={preset.id}
            className={`preset-card ${selectedPreset === preset.id ? 'selected' : ''}`}
            onClick={() => applyPreset(preset.id)}
          >
            <h4>{preset.name}</h4>
            <p>{preset.description}</p>
            <div className="preset-details">
              {getPresetDetails(preset.id)}
            </div>
          </Card>
        ))}
      </div>
    </div>
  );
};

const getPresetDetails = (presetId: string) => {
  const details = {
    ben: [
      'Workers: 1024',
      'Timeout: 60s',
      'Ignores robots.txt',
      'Performance mode',
    ],
    polite: [
      'Workers: 32',
      'Timeout: 10s',
      'Respects robots.txt',
      'Steady mode',
    ],
    stealth: [
      'Workers: 4',
      'Timeout: 30s',
      'Respects robots.txt',
      'Stealth mode',
    ],
  };

  return (
    <ul>
      {details[presetId]?.map((detail, index) => (
        <li key={index}>{detail}</li>
      ))}
    </ul>
  );
};
```

### Proxy Configuration

```typescript
// Proxy configuration interface
export const ProxyConfiguration: React.FC<{ jobId: string }> = ({ jobId }) => {
  const [proxyUrl, setProxyUrl] = useState('');
  const [isProxyEnabled, setIsProxyEnabled] = useState(false);

  const handleProxyToggle = async (enabled: boolean) => {
    try {
      if (enabled && !proxyUrl) {
        toast.error('Please enter a proxy URL first');
        return;
      }

      await scraperAPI.updateJobConfig(jobId, {
        proxy: enabled ? proxyUrl : null,
      });
      
      setIsProxyEnabled(enabled);
      toast.success(enabled ? 'Proxy enabled' : 'Proxy disabled');
    } catch (error) {
      toast.error('Failed to update proxy settings');
    }
  };

  const handleProxyUrlChange = async (url: string) => {
    setProxyUrl(url);
    
    if (isProxyEnabled && url) {
      try {
        await scraperAPI.updateJobConfig(jobId, {
          proxy: url,
        });
      } catch (error) {
        toast.error('Failed to update proxy URL');
      }
    }
  };

  return (
    <div className="proxy-configuration">
      <h3>Proxy Configuration</h3>
      <Toggle
        checked={isProxyEnabled}
        onChange={handleProxyToggle}
        label="Enable Proxy"
      />
      
      {isProxyEnabled && (
        <div className="proxy-settings">
          <Input
            type="url"
            placeholder="http://user:pass@proxy:port"
            value={proxyUrl}
            onChange={(e) => handleProxyUrlChange(e.target.value)}
            className="proxy-url-input"
          />
          <Button
            onClick={() => testProxy(proxyUrl)}
            variant="secondary"
          >
            Test Connection
          </Button>
        </div>
      )}
    </div>
  );
};

const testProxy = async (url: string) => {
  try {
    const result = await scraperAPI.testProxy(url);
    if (result.success) {
      toast.success('Proxy connection successful');
    } else {
      toast.error(`Proxy test failed: ${result.error}`);
    }
  } catch (error) {
    toast.error('Failed to test proxy connection');
  }
};
```

### Real-Time Metrics Visualization

```typescript
// Real-time metrics widget
export const ScraperMetricsWidget: React.FC<{ jobId: string }> = ({ jobId }) => {
  const [metrics, setMetrics] = useState<ScraperMetrics>({
    pagesScraped: 0,
    requestsPerSecond: 0,
    averageResponseTime: 0,
    errorRate: 0,
    memoryUsage: 0,
    cpuUsage: 0,
  });

  const [status, setStatus] = useState<ScraperStatus>('stopped');
  const [isRealTimeActive, setIsRealTimeActive] = useState(true);

  useEffect(() => {
    if (!isRealTimeActive) return;

    const wsClient = new ScraperWebSocketClient('ws://localhost:8080');
    
    wsClient.subscribeToMetrics(jobId, (newMetrics) => {
      setMetrics(newMetrics);
    });

    wsClient.subscribeToStatus(jobId, (newStatus) => {
      setStatus(newStatus.state);
    });

    return () => {
      wsClient.disconnect();
    };
  }, [jobId, isRealTimeActive]);

  return (
    <div className="scraper-metrics-widget">
      <div className="widget-header">
        <h3>Real-Time Metrics</h3>
        <Toggle
          checked={isRealTimeActive}
          onChange={setIsRealTimeActive}
          label="Live Updates"
        />
      </div>
      
      <div className="metrics-grid">
        <MetricCard
          title="Pages Scraped"
          value={metrics.pagesScraped}
          trend={calculateTrend(metrics.pagesScraped)}
        />
        
        <MetricCard
          title="Requests/Second"
          value={metrics.requestsPerSecond}
          unit="req/s"
          trend={calculateTrend(metrics.requestsPerSecond)}
        />
        
        <MetricCard
          title="Avg Response Time"
          value={metrics.averageResponseTime}
          unit="ms"
          trend={calculateTrend(metrics.averageResponseTime)}
        />
        
        <MetricCard
          title="Error Rate"
          value={metrics.errorRate}
          unit="%"
          status={metrics.errorRate > 5 ? 'error' : 'success'}
        />
        
        <MetricCard
          title="Memory Usage"
          value={metrics.memoryUsage}
          unit="MB"
          trend={calculateTrend(metrics.memoryUsage)}
        />
        
        <MetricCard
          title="CPU Usage"
          value={metrics.cpuUsage}
          unit="%"
          status={metrics.cpuUsage > 80 ? 'warning' : 'success'}
        />
      </div>
      
      <div className="status-indicator">
        <StatusIndicator status={status} />
        <span>{status.charAt(0).toUpperCase() + status.slice(1)}</span>
      </div>
    </div>
  );
};
```

### Job Queue Management

```typescript
// Job queue management interface
export const JobQueueManager: React.FC = () => {
  const [jobs, setJobs] = useState<ScraperJob[]>([]);
  const [selectedJob, setSelectedJob] = useState<string | null>(null);

  useEffect(() => {
    loadJobs();
    
    // Set up real-time updates
    const wsClient = new ScraperWebSocketClient('ws://localhost:8080');
    wsClient.subscribeToJobUpdates((update) => {
      setJobs((prevJobs) => 
        prevJobs.map((job) => 
          job.id === update.jobId ? { ...job, ...update.data } : job
        )
      );
    });

    return () => wsClient.disconnect();
  }, []);

  const loadJobs = async () => {
    try {
      const jobList = await scraperAPI.listJobs();
      setJobs(jobList);
    } catch (error) {
      toast.error('Failed to load jobs');
    }
  };

  const handleJobAction = async (jobId: string, action: 'pause' | 'resume' | 'stop') => {
    try {
      switch (action) {
        case 'pause':
          await scraperAPI.pauseJob(jobId);
          toast.success('Job paused');
          break;
        case 'resume':
          await scraperAPI.resumeJob(jobId);
          toast.success('Job resumed');
          break;
        case 'stop':
          await scraperAPI.stopJob(jobId);
          toast.success('Job stopped');
          break;
      }
    } catch (error) {
      toast.error(`Failed to ${action} job`);
    }
  };

  return (
    <div className="job-queue-manager">
      <h3>Job Queue</h3>
      
      <div className="job-list">
        {jobs.map((job) => (
          <Card
            key={job.id}
            className={`job-item ${job.status}`}
            onClick={() => setSelectedJob(job.id)}
          >
            <div className="job-header">
              <h4>{job.url}</h4>
              <StatusIndicator status={job.status} />
            </div>
            
            <div className="job-details">
              <div className="detail-row">
                <span>Status:</span>
                <span className="status">{job.status}</span>
              </div>
              
              {job.progress && (
                <div className="detail-row">
                  <span>Progress:</span>
                  <ProgressBar
                    value={job.progress.completed}
                    max={job.progress.total}
                    showPercentage
                  />
                </div>
              )}
              
              <div className="detail-row">
                <span>Workers:</span>
                <span>{job.config.workers}</span>
              </div>
              
              <div className="detail-row">
                <span>Timeout:</span>
                <span>{job.config.timeout}s</span>
              </div>
            </div>
            
            <div className="job-actions">
              {job.status === 'running' && (
                <>
                  <Button
                    onClick={() => handleJobAction(job.id, 'pause')}
                    variant="secondary"
                    size="small"
                  >
                    Pause
                  </Button>
                  <Button
                    onClick={() => handleJobAction(job.id, 'stop')}
                    variant="danger"
                    size="small"
                  >
                    Stop
                  </Button>
                </>
              )}
              
              {job.status === 'paused' && (
                <Button
                  onClick={() => handleJobAction(job.id, 'resume')}
                  variant="primary"
                  size="small"
                >
                  Resume
                </Button>
              )}
              
              {job.status === 'completed' && (
                <Button
                  onClick={() => downloadResults(job.id)}
                  variant="secondary"
                  size="small"
                >
                  Download
                </Button>
              )}
            </div>
          </Card>
        ))}
      </div>
    </div>
  );
};
```

## Error Handling and Recovery

### Error Display System

```typescript
// Error handling for scraper operations
export const ScraperErrorHandler: React.FC<{ jobId: string }> = ({ jobId }) => {
  const [errors, setErrors] = useState<ScraperError[]>([]);

  useEffect(() => {
    const wsClient = new ScraperWebSocketClient('ws://localhost:8080');
    
    wsClient.subscribeToErrors(jobId, (error) => {
      setErrors((prevErrors) => [...prevErrors, error]);
    });

    return () => wsClient.disconnect();
  }, [jobId]);

  if (errors.length === 0) return null;

  return (
    <div className="scraper-error-handler">
      <h3>Errors</h3>
      
      <div className="error-list">
        {errors.map((error, index) => (
          <Alert
            key={index}
            type="error"
            title={error.type}
            description={error.message}
            actions={[
              {
                label: 'Retry',
                onClick: () => handleRetry(error),
              },
              {
                label: 'Dismiss',
                onClick: () => dismissError(index),
              },
            ]}
          />
        ))}
      </div>
    </div>
  );
};
```

## Performance Monitoring

### Metrics Collection

```rust
// Rust metrics collection
use std::sync::Arc;
use tokio::sync::Mutex;
use std::time::Instant;

pub struct MetricsCollector {
    metrics: Arc<Mutex<ScraperMetrics>>,
    start_time: Instant,
}

#[derive(Debug, Clone)]
pub struct ScraperMetrics {
    pub pages_scraped: u64,
    pub requests_sent: u64,
    pub responses_received: u64,
    pub errors_encountered: u64,
    pub bytes_downloaded: u64,
    pub average_response_time: Duration,
    pub current_concurrency: u32,
    pub queue_size: usize,
}

impl MetricsCollector {
    pub fn new() -> Self {
        Self {
            metrics: Arc::new(Mutex::new(ScraperMetrics {
                pages_scraped: 0,
                requests_sent: 0,
                responses_received: 0,
                errors_encountered: 0,
                bytes_downloaded: 0,
                average_response_time: Duration::from_secs(0),
                current_concurrency: 0,
                queue_size: 0,
            })),
            start_time: Instant::now(),
        }
    }

    pub async fn record_page_scraped(&self) {
        let mut metrics = self.metrics.lock().await;
        metrics.pages_scraped += 1;
    }

    pub async fn record_request_sent(&self) {
        let mut metrics = self.metrics.lock().await;
        metrics.requests_sent += 1;
    }

    pub async fn record_response_received(&self, response_time: Duration, bytes: u64) {
        let mut metrics = self.metrics.lock().await;
        metrics.responses_received += 1;
        metrics.bytes_downloaded += bytes;
        
        // Update average response time
        let total_time = metrics.average_response_time.mul_f32(metrics.responses_received as f32 - 1.0);
        metrics.average_response_time = (total_time + response_time) / metrics.responses_received as u32;
    }

    pub async fn get_metrics(&self) -> ScraperMetrics {
        self.metrics.lock().await.clone()
    }

    pub async fn get_requests_per_second(&self) -> f64 {
        let metrics = self.metrics.lock().await;
        let elapsed = self.start_time.elapsed();
        metrics.requests_sent as f64 / elapsed.as_secs_f64()
    }
}
```

This integration provides comprehensive control and monitoring capabilities for the Rust scraper through the AEGIS Dashboard, enabling users to manage complex scraping operations with real-time feedback and advanced configuration options.