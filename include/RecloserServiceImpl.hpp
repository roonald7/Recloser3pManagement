#pragma once

#include "RecloserManager.hpp"
#include "recloser.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <map>
#include <memory>
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
  explicit RecloserServiceImpl(RecloserManager *manager);

  grpc::Status GetServiceTree(grpc::ServerContext *context,
                              const ServiceTreeRequest *request,
                              ServiceTreeResponse *response) override;

  grpc::Status
  CompareServiceTrees(grpc::ServerContext *context,
                      const CompareServiceTreesRequest *request,
                      CompareServiceTreesResponse *response) override;

  grpc::Status GetScreenLayout(grpc::ServerContext *context,
                               const ScreenLayoutRequest *request,
                               ScreenLayoutResponse *response) override;

  grpc::Status GetFullInventory(grpc::ServerContext *context,
                                const FullInventoryRequest *request,
                                FullInventoryResponse *response) override;

  // CRUD Operations
  grpc::Status CreateRecloser(grpc::ServerContext *context,
                              const RecloserRecord *request,
                              GenericResponse *response) override;
  grpc::Status UpdateRecloser(grpc::ServerContext *context,
                              const RecloserRecord *request,
                              GenericResponse *response) override;
  grpc::Status DeleteRecloser(grpc::ServerContext *context,
                              const DeleteRequest *request,
                              GenericResponse *response) override;

  grpc::Status CreateFirmware(grpc::ServerContext *context,
                              const FirmwareRecord *request,
                              GenericResponse *response) override;
  grpc::Status UpdateFirmware(grpc::ServerContext *context,
                              const FirmwareRecord *request,
                              GenericResponse *response) override;
  grpc::Status DeleteFirmware(grpc::ServerContext *context,
                              const DeleteRequest *request,
                              GenericResponse *response) override;

  grpc::Status AddServiceNode(grpc::ServerContext *context,
                              const ServiceRecord *request,
                              GenericResponse *response) override;
  grpc::Status UpdateServiceNode(grpc::ServerContext *context,
                                 const ServiceRecord *request,
                                 GenericResponse *response) override;
  grpc::Status DeleteServiceNode(grpc::ServerContext *context,
                                 const DeleteRequest *request,
                                 GenericResponse *response) override;

  grpc::Status CreateFeature(grpc::ServerContext *context,
                             const FeatureRecord *request,
                             GenericResponse *response) override;
  grpc::Status UpdateFeature(grpc::ServerContext *context,
                             const FeatureRecord *request,
                             GenericResponse *response) override;
  grpc::Status DeleteFeature(grpc::ServerContext *context,
                             const DeleteRequest *request,
                             GenericResponse *response) override;

private:
  RecloserManager *manager_;

  // Helper to build service tree recursively
  void buildServiceNode(int parentId, int firmwareId, ServiceNode *node);

  // Helper to build internal tree structure for comparison
  void buildInternalTree(int parentId, int firmwareId,
                         const std::string &languageCode,
                         std::map<std::string, ServiceTreeNode> &tree);

  // Helper to compare two trees recursively
  void compareNodes(
      const std::map<std::string, ServiceTreeNode> &tree1,
      const std::map<std::string, ServiceTreeNode> &tree2,
      google::protobuf::RepeatedPtrField<ServiceDifference> *differences,
      int &added, int &removed, int &modified);

  // Helper to build screen layout recursively
  void populateServiceLayout(const RecloserManager::ServiceLayoutRecord &rec,
                             ServiceLayout *layout);
};

} // namespace recloser
