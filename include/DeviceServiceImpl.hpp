#pragma once

#include "DeviceManager.hpp"
#include "device.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <map>
#include <memory>
#include <set>

namespace device {

// Helper structure for comparison
struct ServiceTreeNode {
  std::string description_key;
  std::string display_name;
  std::set<std::string> features;
  std::map<std::string, ServiceTreeNode> children;
};

class DeviceServiceImpl final : public DeviceService::Service {
public:
  explicit DeviceServiceImpl(DeviceManager *manager);

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
  grpc::Status CreateDevice(grpc::ServerContext *context,
                            const DeviceRecord *request,
                            GenericResponse *response) override;
  grpc::Status UpdateDevice(grpc::ServerContext *context,
                            const DeviceRecord *request,
                            GenericResponse *response) override;
  grpc::Status DeleteDevice(grpc::ServerContext *context,
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

  grpc::Status GetLanguages(grpc::ServerContext *context,
                            const LanguagesRequest *request,
                            LanguagesResponse *response) override;

  grpc::Status
  GetDeviceInformation(grpc::ServerContext *context,
                       const DeviceInformationRequest *request,
                       DeviceInformationResponse *response) override;

private:
  DeviceManager *manager_;

  // Helper to build service tree recursively
  void buildServiceNode(int parentId, int dmfId, ServiceNode *node);

  // Helper to build internal tree structure for comparison
  void buildInternalTree(int parentId, int dmfId,
                         const std::string &languageCode,
                         std::map<std::string, ServiceTreeNode> &tree);

  // Helper to compare two trees recursively
  void compareNodes(
      const std::map<std::string, ServiceTreeNode> &tree1,
      const std::map<std::string, ServiceTreeNode> &tree2,
      google::protobuf::RepeatedPtrField<ServiceDifference> *differences,
      int &added, int &removed, int &modified);

  // Helper to build screen layout recursively
  void populateServiceLayout(const DeviceManager::ServiceLayoutRecord &rec,
                             ServiceLayout *layout);
  void populateFeatureDetail(const DeviceManager::FeatureComponentRecord &rec,
                             FeatureComponentDetail *detail);
};

} // namespace device
