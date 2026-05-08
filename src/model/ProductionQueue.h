#pragma once
#include <string>

struct ProductionItem {
    int queueId;
    std::string orderId;
    std::string sampleId;
    int shortage;
    int actualProduction;
    double estimatedTime;
};
