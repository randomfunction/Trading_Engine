#pragma once
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/gauge.h>
#include <memory>

class Metrics {
public:
    static Metrics& instance() {
        static Metrics inst;
        return inst;
    }

    std::shared_ptr<prometheus::Registry> registry;
    
    prometheus::Family<prometheus::Counter>& orders_family;
    prometheus::Counter& orders_received;
    
    prometheus::Family<prometheus::Counter>& trades_family;
    prometheus::Counter& trades_executed;
    
    prometheus::Family<prometheus::Gauge>& depth_family;
    prometheus::Gauge& order_book_depth;
    
    prometheus::Family<prometheus::Histogram>& latency_family;
    prometheus::Histogram& match_latency;

private:
    Metrics() : 
        registry(std::make_shared<prometheus::Registry>()),
        orders_family(prometheus::BuildCounter().Name("trading_orders_total").Help("Total orders").Register(*registry)),
        orders_received(orders_family.Add({})),
        trades_family(prometheus::BuildCounter().Name("trading_trades_total").Help("Total matches").Register(*registry)),
        trades_executed(trades_family.Add({})),
        depth_family(prometheus::BuildGauge().Name("trading_order_book_depth").Help("Active orders").Register(*registry)),
        order_book_depth(depth_family.Add({})),
        latency_family(prometheus::BuildHistogram().Name("trading_match_latency_seconds").Help("Matching latency").Register(*registry)),
        match_latency(latency_family.Add({}, prometheus::Histogram::BucketBoundaries{0.000001, 0.00001, 0.0001, 0.001, 0.01})) 
    {}
};
