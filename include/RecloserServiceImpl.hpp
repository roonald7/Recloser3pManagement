#pragma once

#include <grpcpp/grpcpp.h>
#include "recloser.grpc.pb.h"
#include "RecloserManager.hpp"
#include <memory>
#include <map>
#include <set>

namespace recloser {

// Helper structure for comparison
struct ServiceTreeNode {
    std::string service_key;
    std::string display_name;
    std::set<std::string> features;
    std::map<std::string, ServiceTreeNode> children;
};

class RecloserServiceImpl final : public RecloserService::Service {
public:
    explicit RecloserServiceImpl(RecloserManager* manager);

    grpc::Status GetServiceTree(
        grpc::ServerContext* context,
        const ServiceTreeRequest* request,
        ServiceTreeResponse* response) override;

    grpc::Status CompareServiceTrees(
        grpc::ServerContext* context,
        const CompareServiceTreesRequest* request,
        CompareServiceTreesResponse* response) override;

private:
    RecloserManager* manager_;
    
    // Helper to build service tree recursively
    void buildServiceNode(
        int parentId, 
        int firmwareId, 
        const std::string& languageCode,
        ServiceNode* node);
    
    // Helper to build internal tree structure for comparison
    void buildInternalTree(
        int parentId,
        int firmwareId,
        const std::string& languageCode,
        std::map<std::string, ServiceTreeNode>& tree);
    
    // Helper to compare two trees recursively
    void compareNodes(
        const std::map<std::string, ServiceTreeNode>& tree1,
        const std::map<std::string, ServiceTreeNode>& tree2,
        google::protobuf::RepeatedPtrField<ServiceDifference>* differences,
        int& added, int& removed, int& modified);
};

} // namespace recloser
