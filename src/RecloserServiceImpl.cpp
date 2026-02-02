#include "RecloserServiceImpl.hpp"
#include <iostream>
#include <sstream>

namespace recloser {

RecloserServiceImpl::RecloserServiceImpl(RecloserManager* manager)
    : manager_(manager) {}

grpc::Status RecloserServiceImpl::GetServiceTree(
    grpc::ServerContext* context,
    const ServiceTreeRequest* request,
    ServiceTreeResponse* response) {
    
    int firmwareId = request->firmware_id();
    std::string languageCode = request->language_code();
    
    std::cout << "GetServiceTree called for firmware_id=" << firmwareId 
              << ", language=" << languageCode << std::endl;

    // Get top-level services (parent_id = 0)
    auto topLevelServices = manager_->getServicesByParentAndFirmware(0, firmwareId);
    
    for (const auto& service : topLevelServices) {
        ServiceNode* node = response->add_top_level_services();
        node->set_id(service.id);
        node->set_service_key(service.service_key);
        node->set_display_name(manager_->getTranslation(service.description_key, languageCode));
        
        // Get features for this service
        auto features = manager_->getFeaturesByService(service.id);
        for (const auto& feat : features) {
            Feature* feature = node->add_features();
            feature->set_id(feat.id);
            feature->set_name(feat.description_key);
            feature->set_description(manager_->getTranslation(feat.description_key, languageCode));
        }
        
        // Recursively build children
        buildServiceNode(service.id, firmwareId, languageCode, node);
    }
    
    return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::CompareServiceTrees(
    grpc::ServerContext* context,
    const CompareServiceTreesRequest* request,
    CompareServiceTreesResponse* response) {
    
    int firmwareId1 = request->firmware_id_1();
    int firmwareId2 = request->firmware_id_2();
    std::string languageCode = request->language_code();
    
    std::cout << "CompareServiceTrees called for firmware_id_1=" << firmwareId1 
              << ", firmware_id_2=" << firmwareId2 
              << ", language=" << languageCode << std::endl;
    
    response->set_firmware_id_1(firmwareId1);
    response->set_firmware_id_2(firmwareId2);
    
    // Build internal tree structures for both firmwares
    std::map<std::string, ServiceTreeNode> tree1, tree2;
    buildInternalTree(0, firmwareId1, languageCode, tree1);
    buildInternalTree(0, firmwareId2, languageCode, tree2);
    
    // Compare the trees
    int added = 0, removed = 0, modified = 0;
    compareNodes(tree1, tree2, response->mutable_differences(), added, removed, modified);
    
    // Generate summary
    std::ostringstream summary;
    summary << added << " service(s) added, " 
            << removed << " service(s) removed, " 
            << modified << " service(s) modified";
    response->set_summary(summary.str());
    
    return grpc::Status::OK;
}

void RecloserServiceImpl::buildServiceNode(
    int parentId, 
    int firmwareId, 
    const std::string& languageCode,
    ServiceNode* parentNode) {
    
    auto childServices = manager_->getServicesByParentAndFirmware(parentId, firmwareId);
    
    for (const auto& service : childServices) {
        ServiceNode* childNode = parentNode->add_children();
        childNode->set_id(service.id);
        childNode->set_service_key(service.service_key);
        childNode->set_display_name(manager_->getTranslation(service.description_key, languageCode));
        
        // Get features for this child service
        auto features = manager_->getFeaturesByService(service.id);
        for (const auto& feat : features) {
            Feature* feature = childNode->add_features();
            feature->set_id(feat.id);
            feature->set_name(feat.description_key);
            feature->set_description(manager_->getTranslation(feat.description_key, languageCode));
        }
        
        // Recursively build grandchildren
        buildServiceNode(service.id, firmwareId, languageCode, childNode);
    }
}

void RecloserServiceImpl::buildInternalTree(
    int parentId,
    int firmwareId,
    const std::string& languageCode,
    std::map<std::string, ServiceTreeNode>& tree) {
    
    auto services = manager_->getServicesByParentAndFirmware(parentId, firmwareId);
    
    for (const auto& service : services) {
        ServiceTreeNode node;
        node.service_key = service.service_key;
        node.display_name = manager_->getTranslation(service.description_key, languageCode);
        
        // Get features
        auto features = manager_->getFeaturesByService(service.id);
        for (const auto& feat : features) {
            node.features.insert(feat.description_key);
        }
        
        // Recursively build children
        buildInternalTree(service.id, firmwareId, languageCode, node.children);
        
        tree[service.service_key] = node;
    }
}

void RecloserServiceImpl::compareNodes(
    const std::map<std::string, ServiceTreeNode>& tree1,
    const std::map<std::string, ServiceTreeNode>& tree2,
    google::protobuf::RepeatedPtrField<ServiceDifference>* differences,
    int& added, int& removed, int& modified) {
    
    // Find services in tree1 (check for removed or modified)
    for (const auto& [key, node1] : tree1) {
        auto it2 = tree2.find(key);
        
        if (it2 == tree2.end()) {
            // Service removed in tree2
            ServiceDifference* diff = differences->Add();
            diff->set_service_key(node1.service_key);
            diff->set_display_name(node1.display_name);
            diff->set_difference_type(DifferenceType::REMOVED);
            removed++;
        } else {
            // Service exists in both, check for modifications
            const auto& node2 = it2->second;
            bool hasChanges = false;
            
            ServiceDifference* diff = differences->Add();
            diff->set_service_key(node1.service_key);
            diff->set_display_name(node1.display_name);
            
            // Compare features
            for (const auto& feat : node1.features) {
                if (node2.features.find(feat) == node2.features.end()) {
                    FeatureDifference* featDiff = diff->add_feature_differences();
                    featDiff->set_feature_name(feat);
                    featDiff->set_difference_type(DifferenceType::REMOVED);
                    hasChanges = true;
                }
            }
            
            for (const auto& feat : node2.features) {
                if (node1.features.find(feat) == node1.features.end()) {
                    FeatureDifference* featDiff = diff->add_feature_differences();
                    featDiff->set_feature_name(feat);
                    featDiff->set_difference_type(DifferenceType::ADDED);
                    hasChanges = true;
                }
            }
            
            // Recursively compare children
            int childAdded = 0, childRemoved = 0, childModified = 0;
            compareNodes(node1.children, node2.children, 
                        diff->mutable_child_differences(), 
                        childAdded, childRemoved, childModified);
            
            if (hasChanges || childAdded > 0 || childRemoved > 0 || childModified > 0) {
                diff->set_difference_type(DifferenceType::MODIFIED);
                modified++;
            } else {
                diff->set_difference_type(DifferenceType::UNCHANGED);
                // Remove the diff if nothing changed
                differences->RemoveLast();
            }
        }
    }
    
    // Find services added in tree2
    for (const auto& [key, node2] : tree2) {
        if (tree1.find(key) == tree1.end()) {
            ServiceDifference* diff = differences->Add();
            diff->set_service_key(node2.service_key);
            diff->set_display_name(node2.display_name);
            diff->set_difference_type(DifferenceType::ADDED);
            
            // Add all features as new
            for (const auto& feat : node2.features) {
                FeatureDifference* featDiff = diff->add_feature_differences();
                featDiff->set_feature_name(feat);
                featDiff->set_difference_type(DifferenceType::ADDED);
            }
            
            added++;
        }
    }
}

} // namespace recloser
