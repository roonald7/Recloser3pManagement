#pragma once

#include "DeviceManager.hpp"
#include "ServiceHelpers.hpp"
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

  grpc::Status CreateScreenFeature(grpc::ServerContext *context,
                                   const ScreenFeatureRecord *request,
                                   GenericResponse *response) override;
  grpc::Status UpdateScreenFeature(grpc::ServerContext *context,
                                   const ScreenFeatureRecord *request,
                                   GenericResponse *response) override;
  grpc::Status DeleteScreenFeature(grpc::ServerContext *context,
                                   const DeleteRequest *request,
                                   GenericResponse *response) override;

  grpc::Status GetLanguages(grpc::ServerContext *context,
                            const LanguagesRequest *request,
                            LanguagesResponse *response) override;

  grpc::Status
  GetDeviceInformation(grpc::ServerContext *context,
                       const DeviceInformationRequest *request,
                       DeviceInformationResponse *response) override;
  grpc::Status
  ComparePhysicalDevices(grpc::ServerContext *context,
                         const ComparePhysicalDevicesRequest *request,
                         ComparePhysicalDevicesResponse *response) override;
  grpc::Status
  GetAllPhysicalDevices(grpc::ServerContext *context,
                        const GetAllPhysicalDevicesRequest *request,
                        GetAllPhysicalDevicesResponse *response) override;
  grpc::Status CreateFullDevice(grpc::ServerContext *context,
                                const CreateFullDeviceRequest *request,
                                CreateFullDeviceResponse *response) override;
  grpc::Status
  CreatePhysicalDevice(grpc::ServerContext *context,
                       const CreatePhysicalDeviceRequest *request,
                       CreatePhysicalDeviceResponse *response) override;
  grpc::Status
  GetPhysicalDevices(grpc::ServerContext *context,
                     const GetPhysicalDevicesRequest *request,
                     GetPhysicalDevicesResponse *response) override;
  grpc::Status UpdatePhysicalDevice(grpc::ServerContext *context,
                                    const UpdatePhysicalDeviceRequest *request,
                                    GenericResponse *response) override;
  grpc::Status DeletePhysicalDevice(grpc::ServerContext *context,
                                    const DeletePhysicalDeviceRequest *request,
                                    GenericResponse *response) override;

private:
  DeviceManager *manager_;

  // Helper to build service tree recursively
  void buildServiceNode(int parentId, int dmfId, ServiceNode *node);
  void fillServiceNode(const ServiceNodeInfo &info, ServiceNode *node);

  // Helper to build internal tree structure for comparison
  void buildInternalTree(int parentId, int dmfId,
                         const std::string &languageCode,
                         std::map<std::string, ServiceTreeNode> &tree);

  // Helper to compare two trees recursively
  void compareNodes(
      const std::map<std::string, ServiceTreeNode> &tree1,
      const std::map<std::string, ServiceTreeNode> &tree2,
      google::protobuf::RepeatedPtrField<ServiceDifference> *differences,
      int &added, int &removed, int &modified, const std::string &languageCode);

  // Helper to build screen layout recursively
  void populateServiceLayout(const DeviceManager::ServiceLayoutRecord &rec,
                             ServiceLayout *layout,
                             const std::map<int, std::string> &savedValues);
  void populateFeatureDetail(const DeviceManager::FeatureComponentRecord &rec,
                             FeatureComponentDetail *detail,
                             const std::map<int, std::string> &savedValues);

  // Helper to build physical device comparison tree
  bool
  buildPhysicalComparisonNode(const ServiceNodeInfo &info, int64_t id1,
                              int64_t id2,
                              const std::map<std::string, std::string> &valMap1,
                              const std::map<std::string, std::string> &valMap2,
                              PhysicalDeviceServiceComparison *node);
};

class RecordServiceImpl final : public RecordService::Service {
public:
  explicit RecordServiceImpl(DeviceManager *manager);

  grpc::Status GetAllRecords(grpc::ServerContext *context,
                             const GetAllRecordsRequest *request,
                             GetAllRecordsResponse *response) override;

private:
  DeviceManager *manager_;
};

} // namespace device
