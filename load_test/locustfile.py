from locust import HttpUser, task, between
import random

class TradingLoadTester(HttpUser):
    # Simulated orders-per-second targeting high throughput
    wait_time = between(0.0001, 0.001)

    @task
    def metrics_check(self):
        # We target the /metrics endpoint as the engine is currently simulated internally
        self.client.get("/metrics")
