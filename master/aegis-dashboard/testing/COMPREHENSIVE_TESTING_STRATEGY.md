# AEGIS Comprehensive Testing Strategy

## Executive Summary

This document outlines the comprehensive testing strategy for the AEGIS Dashboard system, ensuring every component meets the highest standards of quality, performance, and reliability. The testing approach covers unit tests, integration tests, end-to-end tests, performance benchmarks, and security assessments.

## Testing Philosophy

### Core Principles

1. **Test-Driven Development**: Write tests before implementation
2. **Comprehensive Coverage**: 100% code coverage for critical components
3. **Performance Validation**: Every feature must meet performance targets
4. **Security First**: Security testing integrated into every phase
5. **User-Centric**: Tests validate user experience and accessibility

### Testing Pyramid

```
            E2E Tests (10%)
                 /    |    \
        Integration Tests (30%)
              /    |    |    \
    Unit Tests (60%)
```

## Unit Testing Framework

### Frontend Unit Tests

#### Component Testing

```typescript
// MetricCard component test
import { render, screen, fireEvent } from '@testing-library/react';
import { MetricCard } from './MetricCard';

describe('MetricCard Component', () => {
  const mockData = {
    title: 'CPU Usage',
    value: 75.5,
    unit: '%',
    trend: 2.3,
    status: 'warning' as const,
    sparkline: [70, 72, 75, 73, 75.5]
  };

  test('renders metric data correctly', () => {
    render(<MetricCard {...mockData} />);
    
    expect(screen.getByText('CPU Usage')).toBeInTheDocument();
    expect(screen.getByText('75.5')).toBeInTheDocument();
    expect(screen.getByText('%')).toBeInTheDocument();
  });

  test('applies correct status styling', () => {
    render(<MetricCard {...mockData} />);
    
    const card = screen.getByTestId('metric-card');
    expect(card).toHaveClass('status-warning');
  });

  test('displays sparkline when provided', () => {
    render(<MetricCard {...mockData} />);
    
    const sparkline = screen.getByTestId('sparkline');
    expect(sparkline).toBeInTheDocument();
  });

  test('handles click interactions', () => {
    const onClick = jest.fn();
    render(<MetricCard {...mockData} onClick={onClick} />);
    
    fireEvent.click(screen.getByTestId('metric-card'));
    expect(onClick).toHaveBeenCalledTimes(1);
  });

  test('shows tooltip on hover', async () => {
    render(<MetricCard {...mockData} />);
    
    const card = screen.getByTestId('metric-card');
    fireEvent.mouseEnter(card);
    
    const tooltip = await screen.findByTestId('tooltip');
    expect(tooltip).toBeInTheDocument();
    expect(tooltip).toHaveTextContent('Trend: +2.3%');
  });
});
```

#### Hook Testing

```typescript
// Custom hook test
import { renderHook, act } from '@testing-library/react-hooks';
import { useWebSocket } from './useWebSocket';

describe('useWebSocket Hook', () => {
  let mockServer: WebSocketServer;

  beforeEach(() => {
    mockServer = new WebSocketServer('ws://localhost:8080');
  });

  afterEach(() => {
    mockServer.close();
  });

  test('connects to WebSocket server', async () => {
    const { result, waitFor } = renderHook(() => useWebSocket('ws://localhost:8080'));
    
    await waitFor(() => {
      expect(result.current.isConnected).toBe(true);
    });
  });

  test('receives messages correctly', async () => {
    const { result, waitFor } = renderHook(() => useWebSocket('ws://localhost:8080'));
    
    const testMessage = { type: 'metric', data: { value: 42 } };
    mockServer.send(testMessage);
    
    await waitFor(() => {
      expect(result.current.lastMessage).toEqual(testMessage);
    });
  });

  test('handles connection errors', async () => {
    mockServer.simulateError(new Error('Connection failed'));
    
    const { result, waitFor } = renderHook(() => useWebSocket('ws://localhost:8080'));
    
    await waitFor(() => {
      expect(result.current.error).toBeTruthy();
      expect(result.current.isConnected).toBe(false);
    });
  });

  test('reconnects after disconnection', async () => {
    const { result, waitFor } = renderHook(() => useWebSocket('ws://localhost:8080', { autoReconnect: true }));
    
    await waitFor(() => {
      expect(result.current.isConnected).toBe(true);
    });
    
    mockServer.close();
    
    await waitFor(() => {
      expect(result.current.isConnected).toBe(false);
      expect(result.current.reconnectAttempts).toBeGreaterThan(0);
    });
    
    // Restart server
    mockServer = new WebSocketServer('ws://localhost:8080');
    
    await waitFor(() => {
      expect(result.current.isConnected).toBe(true);
    }, { timeout: 10000 });
  });
});
```

#### Utility Function Testing

```typescript
// Data processing utility test
import { processMetrics, calculateTrend, formatBytes } from './dataUtils';

describe('Data Processing Utilities', () => {
  describe('processMetrics', () => {
    test('processes raw metric data correctly', () => {
      const rawData = [
        { timestamp: 1640995200000, value: 100 },
        { timestamp: 1640995260000, value: 120 },
        { timestamp: 1640995320000, value: 110 }
      ];

      const processed = processMetrics(rawData);

      expect(processed).toHaveLength(3);
      expect(processed[0].timestamp).toBeInstanceOf(Date);
      expect(processed[0].trend).toBeDefined();
    });

    test('handles empty data array', () => {
      const processed = processMetrics([]);
      expect(processed).toEqual([]);
    });

    test('filters out invalid data points', () => {
      const rawData = [
        { timestamp: 1640995200000, value: 100 },
        { timestamp: null, value: 120 },
        { timestamp: 1640995320000, value: null }
      ];

      const processed = processMetrics(rawData);
      expect(processed).toHaveLength(1);
    });
  });

  describe('calculateTrend', () => {
    test('calculates positive trend correctly', () => {
      const data = [100, 110, 120, 130];
      const trend = calculateTrend(data);
      expect(trend).toBeCloseTo(10);
    });

    test('calculates negative trend correctly', () => {
      const data = [130, 120, 110, 100];
      const trend = calculateTrend(data);
      expect(trend).toBeCloseTo(-10);
    });

    test('handles single data point', () => {
      const data = [100];
      const trend = calculateTrend(data);
      expect(trend).toBe(0);
    });
  });

  describe('formatBytes', () => {
    test('formats bytes correctly', () => {
      expect(formatBytes(1024)).toBe('1.0 KB');
      expect(formatBytes(1048576)).toBe('1.0 MB');
      expect(formatBytes(1073741824)).toBe('1.0 GB');
    });

    test('handles edge cases', () => {
      expect(formatBytes(0)).toBe('0 B');
      expect(formatBytes(512)).toBe('512 B');
    });
  });
});
```

### Backend Unit Tests (Rust)

```rust
// Rust unit test example
#[cfg(test)]
mod tests {
    use super::*;
    use tokio::test;

    #[test]
    async fn test_metrics_processing() {
        let raw_metrics = vec![
            Metric {
                timestamp: 1640995200000,
                value: 100.0,
                metric_type: MetricType::CpuUsage,
            },
            Metric {
                timestamp: 1640995260000,
                value: 120.0,
                metric_type: MetricType::CpuUsage,
            },
        ];

        let processed = process_metrics(raw_metrics).await;

        assert_eq!(processed.len(), 2);
        assert!(processed[0].trend.is_some());
        assert_eq!(processed[0].metric_type, MetricType::CpuUsage);
    }

    #[test]
    async fn test_websocket_broadcast() {
        let server = WebSocketServer::new("127.0.0.1:8080");
        let mut client1 = WebSocketClient::connect("ws://127.0.0.1:8080").await.unwrap();
        let mut client2 = WebSocketClient::connect("ws://127.0.0.1:8080").await.unwrap();

        // Send message from server
        let test_message = json!({
            "type": "metric_update",
            "data": { "value": 42 }
        });

        server.broadcast(test_message.to_string()).await;

        // Verify both clients received the message
        let msg1 = client1.read().await.unwrap();
        let msg2 = client2.read().await.unwrap();

        assert_eq!(msg1, test_message.to_string());
        assert_eq!(msg2, test_message.to_string());
    }

    #[test]
    async fn test_scraper_job_queue() {
        let queue = JobQueue::new();
        let job = ScraperJob {
            id: Uuid::new_v4(),
            url: "https://example.com".to_string(),
            priority: 1,
            status: JobStatus::Pending,
        };

        // Add job to queue
        queue.push(job.clone()).await;

        // Verify job is in queue
        let queued_jobs = queue.list().await;
        assert_eq!(queued_jobs.len(), 1);
        assert_eq!(queued_jobs[0].id, job.id);

        // Process job
        let processed_job = queue.pop().await.unwrap();
        assert_eq!(processed_job.id, job.id);
        assert_eq!(processed_job.status, JobStatus::Running);
    }
}
```

## Integration Testing

### API Integration Tests

```typescript
// API integration test
import { request } from './testUtils';
import { expect } from '@jest/globals';

describe('Scraper API Integration', () => {
  const baseURL = 'http://localhost:3000/api';

  describe('POST /api/jobs', () => {
    test('creates a new scraping job', async () => {
      const jobConfig = {
        url: 'https://example.com',
        depth: 2,
        workers: 10,
        timeout: 30,
      };

      const response = await request(baseURL)
        .post('/jobs')
        .send(jobConfig);

      expect(response.status).toBe(201);
      expect(response.body).toMatchObject({
        id: expect.any(String),
        status: 'created',
        config: jobConfig,
      });
    });

    test('validates required fields', async () => {
      const response = await request(baseURL)
        .post('/jobs')
        .send({});

      expect(response.status).toBe(400);
      expect(response.body).toMatchObject({
        error: 'ValidationError',
        details: expect.arrayContaining([
          expect.objectContaining({ field: 'url', message: 'required' }),
        ]),
      });
    });

    test('enforces rate limits', async () => {
      const requests = Array(100).fill(null).map(() =>
        request(baseURL).post('/jobs').send({
          url: 'https://example.com',
          depth: 1,
        })
      );

      const responses = await Promise.all(requests);
      const rateLimitedResponses = responses.filter(r => r.status === 429);

      expect(rateLimitedResponses.length).toBeGreaterThan(0);
    });
  });

  describe('GET /api/jobs/:id/status', () => {
    test('retrieves job status correctly', async () => {
      // Create a job first
      const createResponse = await request(baseURL)
        .post('/jobs')
        .send({
          url: 'https://example.com',
          depth: 1,
        });

      const jobId = createResponse.body.id;

      // Check status
      const statusResponse = await request(baseURL)
        .get(`/jobs/${jobId}/status`);

      expect(statusResponse.status).toBe(200);
      expect(statusResponse.body).toMatchObject({
        id: jobId,
        status: expect.any(String),
        progress: expect.objectContaining({
          completed: expect.any(Number),
          total: expect.any(Number),
          percentage: expect.any(Number),
        }),
      });
    });

    test('returns 404 for non-existent job', async () => {
      const fakeId = '550e8400-e29b-41d4-a716-446655440000';
      
      const response = await request(baseURL)
        .get(`/jobs/${fakeId}/status`);

      expect(response.status).toBe(404);
    });
  });

  describe('WebSocket Integration', () => {
    test('receives real-time updates', async () => {
      const ws = new WebSocket('ws://localhost:3000/ws');
      
      const messagePromise = new Promise((resolve) => {
        ws.onmessage = (event) => {
          resolve(JSON.parse(event.data));
        };
      });

      ws.onopen = async () => {
        // Create a job to trigger updates
        await request(baseURL)
          .post('/jobs')
          .send({ url: 'https://example.com', depth: 1 });
      };

      const message = await messagePromise;
      expect(message).toMatchObject({
        type: expect.any(String),
        timestamp: expect.any(Number),
        data: expect.any(Object),
      });

      ws.close();
    });
  });
});
```

### Database Integration Tests

```typescript
// Database integration test
import { db } from './testDatabase';
import { expect } from '@jest/globals';

describe('Database Integration', () => {
  beforeEach(async () => {
    await db.migrate.latest();
    await db.seed.run();
  });

  afterEach(async () => {
    await db.destroy();
  });

  describe('Metrics Storage', () => {
    test('stores and retrieves metrics correctly', async () => {
      const testMetrics = [
        {
          scraperId: 'scraper-1',
          timestamp: new Date(),
          type: 'cpu_usage',
          value: 75.5,
          metadata: { unit: '%' },
        },
        {
          scraperId: 'scraper-1',
          timestamp: new Date(),
          type: 'memory_usage',
          value: 2048,
          metadata: { unit: 'MB' },
        },
      ];

      // Store metrics
      await db('metrics').insert(testMetrics);

      // Retrieve metrics
      const storedMetrics = await db('metrics')
        .where({ scraperId: 'scraper-1' })
        .orderBy('timestamp', 'desc');

      expect(storedMetrics).toHaveLength(2);
      expect(storedMetrics[0]).toMatchObject(testMetrics[1]);
    });

    test('handles time-series data efficiently', async () => {
      const startTime = Date.now();
      const metrics = [];

      // Generate 10000 metric points
      for (let i = 0; i < 10000; i++) {
        metrics.push({
          scraperId: 'scraper-1',
          timestamp: new Date(startTime + i * 1000),
          type: 'cpu_usage',
          value: Math.random() * 100,
        });
      }

      const insertStart = Date.now();
      await db('metrics').insert(metrics);
      const insertDuration = Date.now() - insertStart;

      expect(insertDuration).toBeLessThan(5000); // Should insert in under 5 seconds

      // Query performance
      const queryStart = Date.now();
      const recentMetrics = await db('metrics')
        .where('timestamp', '>', new Date(startTime + 5000000))
        .select('*');
      const queryDuration = Date.now() - queryStart;

      expect(queryDuration).toBeLessThan(100); // Should query in under 100ms
      expect(recentMetrics.length).toBeGreaterThan(0);
    });
  });

  describe('Job Queue Management', () => {
    test('processes job queue correctly', async () => {
      const jobs = [
        { id: 'job-1', url: 'https://example1.com', status: 'pending', priority: 1 },
        { id: 'job-2', url: 'https://example2.com', status: 'pending', priority: 2 },
        { id: 'job-3', url: 'https://example3.com', status: 'pending', priority: 1 },
      ];

      await db('jobs').insert(jobs);

      // Simulate job processing
      await db('jobs')
        .where({ id: 'job-1' })
        .update({ status: 'running', startedAt: new Date() });

      const runningJobs = await db('jobs').where({ status: 'running' });
      expect(runningJobs).toHaveLength(1);
      expect(runningJobs[0].id).toBe('job-1');

      // Complete job
      await db('jobs')
        .where({ id: 'job-1' })
        .update({ 
          status: 'completed', 
          completedAt: new Date(),
          result: { pagesScraped: 100 }
        });

      const completedJobs = await db('jobs').where({ status: 'completed' });
      expect(completedJobs).toHaveLength(1);
    });
  });
});
```

## End-to-End Testing

### Critical User Journey Tests

```typescript
// E2E test using Playwright
import { test, expect } from '@playwright/test';

test.describe('Critical User Journeys', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('http://localhost:3000');
    await page.waitForLoadState('networkidle');
  });

  test('user can create and monitor a scraping job', async ({ page }) => {
    // Navigate to job creation
    await page.click('[data-testid="create-job-button"]');
    
    // Fill job configuration
    await page.fill('[data-testid="url-input"]', 'https://example.com');
    await page.fill('[data-testid="depth-input"]', '2');
    await page.fill('[data-testid="workers-input"]', '5');
    
    // Submit job
    await page.click('[data-testid="submit-job-button"]');
    
    // Wait for job to be created
    await page.waitForSelector('[data-testid="job-created-notification"]');
    
    // Navigate to dashboard
    await page.click('[data-testid="dashboard-link"]');
    
    // Verify job appears in dashboard
    const jobCard = await page.waitForSelector('[data-testid="job-card"]');
    expect(jobCard).toBeTruthy();
    
    // Monitor job progress
    await page.waitForSelector('[data-testid="job-status-running"]');
    
    // Wait for completion
    await page.waitForSelector('[data-testid="job-status-completed"]');
    
    // Verify results
    const results = await page.textContent('[data-testid="job-results"]');
    expect(results).toContain('Pages scraped');
  });

  test('user can configure and test adversarial simulation', async ({ page }) => {
    // Navigate to adversarial simulation
    await page.click('[data-testid="adversarial-tab"]');
    
    // Select simulation scenario
    await page.click('[data-testid="scenario-dropdown"]');
    await page.click('[data-testid="scenario-bot-detection"]');
    
    // Configure simulation parameters
    await page.fill('[data-testid="intensity-slider"]', '75');
    await page.check('[data-testid="enable-waf-checkbox"]');
    
    // Start simulation
    await page.click('[data-testid="start-simulation-button"]');
    
    // Monitor simulation progress
    await page.waitForSelector('[data-testid="simulation-running"]');
    
    // Check real-time metrics
    const cpuMetric = await page.waitForSelector('[data-testid="metric-cpu-usage"]');
    expect(cpuMetric).toBeTruthy();
    
    // Verify error rate increases during simulation
    const errorRate = await page.textContent('[data-testid="error-rate"]');
    expect(parseFloat(errorRate)).toBeGreaterThan(0);
    
    // Stop simulation
    await page.click('[data-testid="stop-simulation-button"]');
    
    // Verify simulation stopped
    await page.waitForSelector('[data-testid="simulation-stopped"]');
  });

  test('user can customize dashboard layout', async ({ page }) => {
    // Initial dashboard state
    const initialWidgets = await page.$$('[data-testid="dashboard-widget"]');
    expect(initialWidgets).toHaveLength(6); // Default layout
    
    // Enter edit mode
    await page.click('[data-testid="edit-layout-button"]');
    
    // Drag widget to new position
    const sourceWidget = await page.$('[data-testid="widget-cpu"]');
    const targetPosition = await page.$('[data-testid="widget-memory"]');
    
    await sourceWidget.dragTo(targetPosition);
    
    // Add new widget
    await page.click('[data-testid="add-widget-button"]');
    await page.click('[data-testid="widget-type-network"]');
    await page.click('[data-testid="confirm-add-widget"]');
    
    // Verify new widget added
    const updatedWidgets = await page.$$('[data-testid="dashboard-widget"]');
    expect(updatedWidgets).toHaveLength(7);
    
    // Save layout
    await page.click('[data-testid="save-layout-button"]');
    
    // Reload page and verify layout persisted
    await page.reload();
    await page.waitForLoadState('networkidle');
    
    const persistedWidgets = await page.$$('[data-testid="dashboard-widget"]');
    expect(persistedWidgets).toHaveLength(7);
  });

  test('responsive design works on different screen sizes', async ({ page }) => {
    // Test mobile layout
    await page.setViewportSize({ width: 375, height: 667 });
    await page.waitForLoadState('networkidle');
    
    // Mobile should show collapsed sidebar
    const sidebar = await page.$('[data-testid="sidebar"]');
    expect(await sidebar.getAttribute('class')).toContain('collapsed');
    
    // Test tablet layout
    await page.setViewportSize({ width: 768, height: 1024 });
    await page.waitForLoadState('networkidle');
    
    // Tablet should show 2-column grid
    const widgets = await page.$$('[data-testid="dashboard-widget"]');
    expect(widgets.length).toBeGreaterThan(0);
    
    // Test desktop layout
    await page.setViewportSize({ width: 1920, height: 1080 });
    await page.waitForLoadState('networkidle');
    
    // Desktop should show full layout
    const expandedSidebar = await page.$('[data-testid="sidebar"]');
    expect(await expandedSidebar.getAttribute('class')).not.toContain('collapsed');
  });
});
```

### Performance Testing

```typescript
// Performance test using Lighthouse
import { test } from '@playwright/test';
import { playAudit } from 'playwright-lighthouse';

test.describe('Performance Testing', () => {
  test('meets performance targets', async ({ page }) => {
    await page.goto('http://localhost:3000');
    
    const lighthouseReport = await playAudit({
      page,
      thresholds: {
        performance: 90,
        accessibility: 95,
        'best-practices': 90,
        seo: 90,
      },
      opts: {
        onlyCategories: ['performance', 'accessibility', 'best-practices', 'seo'],
      },
    });

    expect(lighthouseReport.performance).toBeGreaterThanOrEqual(90);
    expect(lighthouseReport.accessibility).toBeGreaterThanOrEqual(95);
  });

  test('maintains 60fps during interactions', async ({ page }) => {
    await page.goto('http://localhost:3000');
    
    // Start FPS monitoring
    await page.evaluate(() => {
      window.fpsCounter = [];
      let lastTime = performance.now();
      
      function measureFPS() {
        const currentTime = performance.now();
        const deltaTime = currentTime - lastTime;
        const fps = 1000 / deltaTime;
        window.fpsCounter.push(fps);
        lastTime = currentTime;
        
        requestAnimationFrame(measureFPS);
      }
      
      requestAnimationFrame(measureFPS);
    });
    
    // Perform interactions
    await page.dragAndDrop('[data-testid="widget-1"]', '[data-testid="widget-2"]');
    await page.click('[data-testid="refresh-button"]');
    
    // Check FPS after interactions
    const minFPS = await page.evaluate(() => {
      return Math.min(...window.fpsCounter.slice(-100));
    });
    
    expect(minFPS).toBeGreaterThan(55); // Allow for some variance
  });

  test('handles large datasets efficiently', async ({ page }) => {
    await page.goto('http://localhost:3000');
    
    // Load large dataset
    await page.click('[data-testid="load-large-dataset"]');
    
    // Measure load time
    const loadStart = Date.now();
    await page.waitForSelector('[data-testid="dataset-loaded"]');
    const loadTime = Date.now() - loadStart;
    
    expect(loadTime).toBeLessThan(5000); // Should load in under 5 seconds
    
    // Check memory usage
    const memoryUsage = await page.evaluate(() => {
      return performance.memory.usedJSHeapSize / (1024 * 1024);
    });
    
    expect(memoryUsage).toBeLessThan(100); // Should use less than 100MB
  });
});
```

## Security Testing

### Vulnerability Assessment

```typescript
// Security test examples
import { test, expect } from '@playwright/test';

test.describe('Security Testing', () => {
  test('prevents XSS attacks', async ({ page }) => {
    await page.goto('http://localhost:3000');
    
    // Attempt XSS through URL parameter
    await page.goto('http://localhost:3000/dashboard?user=<script>alert(1)</script>');
    
    // Check that script was not executed
    const hasAlert = await page.evaluate(() => {
      return window.alertTriggered;
    });
    
    expect(hasAlert).toBeFalsy();
    
    // Check that content is properly escaped
    const userContent = await page.textContent('[data-testid="user-display"]');
    expect(userContent).toContain('<script>alert(1)</script>');
  });

  test('enforces CSRF protection', async ({ page }) => {
    await page.goto('http://localhost:3000');
    
    // Attempt to make request without CSRF token
    const response = await page.evaluate(async () => {
      return fetch('/api/jobs', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ url: 'https://example.com' }),
      });
    });
    
    expect(response.status).toBe(403);
  });

  test('implements proper authentication', async ({ page }) => {
    // Attempt to access protected resource without authentication
    const response = await page.goto('http://localhost:3000/api/admin/users');
    expect(response?.status()).toBe(401);
    
    // Login and try again
    await page.goto('http://localhost:3000/login');
    await page.fill('[data-testid="username-input"]', 'admin');
    await page.fill('[data-testid="password-input"]', 'password');
    await page.click('[data-testid="login-button"]');
    
    // Should now have access
    const adminResponse = await page.goto('http://localhost:3000/api/admin/users');
    expect(adminResponse?.status()).toBe(200);
  });

  test('prevents SQL injection', async ({ page }) => {
    await page.goto('http://localhost:3000');
    
    // Attempt SQL injection in search
    await page.fill('[data-testid="search-input"]', "'; DROP TABLE users; --");
    await page.click('[data-testid="search-button"]');
    
    // Should handle gracefully without error
    const errorMessage = await page.$('[data-testid="error-message"]');
    expect(errorMessage).toBeFalsy();
  });
});
```

### Authentication & Authorization Testing

```typescript
test.describe('Authentication & Authorization', () => {
  test('role-based access control works correctly', async ({ page }) => {
    // Test admin access
    await loginAs(page, 'admin');
    
    let response = await page.goto('http://localhost:3000/api/admin/users');
    expect(response?.status()).toBe(200);
    
    response = await page.goto('http://localhost:3000/api/jobs');
    expect(response?.status()).toBe(200);
    
    await logout(page);
    
    // Test operator access
    await loginAs(page, 'operator');
    
    response = await page.goto('http://localhost:3000/api/admin/users');
    expect(response?.status()).toBe(403);
    
    response = await page.goto('http://localhost:3000/api/jobs');
    expect(response?.status()).toBe(200);
    
    await logout(page);
    
    // Test viewer access
    await loginAs(page, 'viewer');
    
    response = await page.goto('http://localhost:3000/api/admin/users');
    expect(response?.status()).toBe(403);
    
    response = await page.goto('http://localhost:3000/api/jobs');
    expect(response?.status()).toBe(200);
    
    // Try to create job (should fail)
    const createResponse = await page.evaluate(async () => {
      return fetch('/api/jobs', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ url: 'https://example.com' }),
      });
    });
    
    expect(createResponse.status).toBe(403);
  });

  test('JWT token expiration works correctly', async ({ page }) => {
    // Login to get token
    await loginAs(page, 'admin');
    
    // Wait for token to expire (or manipulate system time)
    await page.evaluate(() => {
      // Fast-forward time by 25 hours
      const now = Date.now();
      jest.advanceTimersByTime(25 * 60 * 60 * 1000);
    });
    
    // Try to access protected resource
    const response = await page.goto('http://localhost:3000/api/admin/users');
    expect(response?.status()).toBe(401);
    
    // Should be redirected to login
    expect(page.url()).toContain('/login');
  });
});
```

## Test Automation & CI/CD

### GitHub Actions Workflow

```yaml
# .github/workflows/test.yml
name: Test Suite

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  test:
    runs-on: ubuntu-latest
    
    services:
      postgres:
        image: postgres:13
        env:
          POSTGRES_PASSWORD: postgres
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
      
      redis:
        image: redis:6
        options: >-
          --health-cmd "redis-cli ping"
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5

    steps:
    - uses: actions/checkout@v3
    
    - name: Setup Node.js
      uses: actions/setup-node@v3
      with:
        node-version: '18'
        cache: 'npm'
    
    - name: Setup Rust
      uses: actions-rs/toolchain@v1
      with:
        toolchain: stable
        override: true
        components: rustfmt, clippy
    
    - name: Install dependencies
      run: |
        npm ci
        cargo build --release
    
    - name: Run linter
      run: |
        npm run lint
        cargo clippy -- -D warnings
    
    - name: Run unit tests
      run: |
        npm run test:unit
        cargo test
    
    - name: Run integration tests
      env:
        DATABASE_URL: postgresql://postgres:postgres@localhost:5432/test
        REDIS_URL: redis://localhost:6379
      run: |
        npm run test:integration
        cargo test --test integration_tests
    
    - name: Run E2E tests
      run: |
        npm run start:prod &
        sleep 30
        npm run test:e2e
    
    - name: Run performance tests
      run: |
        npm run test:performance
    
    - name: Generate coverage report
      run: |
        npm run test:coverage
        cargo tarpaulin --out Xml
    
    - name: Upload coverage to Codecov
      uses: codecov/codecov-action@v3
      with:
        files: ./coverage.xml,./tarpaulin-report.xml
```

### Test Execution Script

```bash
#!/bin/bash
# test.sh - Comprehensive test execution script

set -e

echo "🧪 Starting AEGIS Test Suite"
echo "=============================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print status
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v node &> /dev/null; then
    print_error "Node.js is not installed"
    exit 1
fi

if ! command -v cargo &> /dev/null; then
    print_error "Rust/Cargo is not installed"
    exit 1
fi

print_status "Prerequisites check passed"

# Install dependencies
echo -e "\nInstalling dependencies..."
npm ci
print_status "Frontend dependencies installed"

# Run linter
echo -e "\nRunning linter..."
if npm run lint; then
    print_status "Linting passed"
else
    print_error "Linting failed"
    exit 1
fi

# Run unit tests
echo -e "\nRunning unit tests..."
if npm run test:unit; then
    print_status "Frontend unit tests passed"
else
    print_error "Frontend unit tests failed"
    exit 1
fi

# Run Rust tests
echo -e "\nRunning Rust tests..."
if cargo test; then
    print_status "Rust tests passed"
else
    print_error "Rust tests failed"
    exit 1
fi

# Run integration tests
echo -e "\nRunning integration tests..."
if npm run test:integration; then
    print_status "Integration tests passed"
else
    print_error "Integration tests failed"
    exit 1
fi

# Run E2E tests
echo -e "\nRunning E2E tests..."
if npm run test:e2e; then
    print_status "E2E tests passed"
else
    print_error "E2E tests failed"
    exit 1
fi

# Run performance tests
echo -e "\nRunning performance tests..."
if npm run test:performance; then
    print_status "Performance tests passed"
else
    print_warning "Performance tests failed (non-critical)"
fi

# Run security tests
echo -e "\nRunning security tests..."
if npm run test:security; then
    print_status "Security tests passed"
else
    print_error "Security tests failed"
    exit 1
fi

echo -e "\n🎉 All tests completed successfully!"
echo "=============================="
```

This comprehensive testing strategy ensures that every component of the AEGIS Dashboard meets the highest standards of quality, performance, and security. The multi-layered approach provides confidence in the system's reliability and user experience.