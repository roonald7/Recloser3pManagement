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
  std::vector<TranslationRecord> translations;
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
  grpc::Status CreateLine(grpc::ServerContext *context,
                          const LineRecord *request,
                          GenericResponse *response) override;
  grpc::Status UpdateLine(grpc::ServerContext *context,
                          const LineRecord *request,
                          GenericResponse *response) override;
  grpc::Status DeleteLine(grpc::ServerContext *context,
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

  grpc::Status GetLineInformation(grpc::ServerContext *context,
                                  const LineInformationRequest *request,
                                  LineInformationResponse *response) override;
  grpc::Status CompareDevices(grpc::ServerContext *context,
                              const CompareDevicesRequest *request,
                              CompareDevicesResponse *response) override;
  grpc::Status GetAllDevices(grpc::ServerContext *context,
                             const GetAllDevicesRequest *request,
                             GetAllDevicesResponse *response) override;
  grpc::Status CreateFullLine(grpc::ServerContext *context,
                              const CreateFullLineRequest *request,
                              CreateFullLineResponse *response) override;
  grpc::Status CreateDevice(grpc::ServerContext *context,
                            const CreateDeviceRequest *request,
                            CreateDeviceResponse *response) override;
  grpc::Status ListLineDevices(grpc::ServerContext *context,
                               const ListLineDevicesRequest *request,
                               ListLineDeviceResponse *response) override;
  grpc::Status GetDevices(grpc::ServerContext *context,
                          const GetDevicesRequest *request,
                          GetDevicesResponse *response) override;
  grpc::Status UpdateDevice(grpc::ServerContext *context,
                            const UpdateDeviceRequest *request,
                            GenericResponse *response) override;
  grpc::Status DeleteDevice(grpc::ServerContext *context,
                            const DeleteDeviceRequest *request,
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

  // Helper to build device comparison tree
  bool buildComparisonNode(const ServiceNodeInfo &info, int64_t id1,
                           int64_t id2,
                           const std::map<std::string, std::string> &valMap1,
                           const std::map<std::string, std::string> &valMap2,
                           DeviceServiceComparison *node);
};

class RecordServiceImpl final : public RecordService::Service {
public:
  explicit RecordServiceImpl(DeviceManager *manager);

  grpc::Status GetAllRecords(grpc::ServerContext *context,
                             const GetAllRecordsRequest *request,
                             GetAllRecordsResponse *response) override;

  grpc::Status ListDeviceRecords(grpc::ServerContext *context,
                                 const ListDeviceRecordsRequest *request,
                                 ListDeviceRecordsResponse *response) override;

  grpc::Status SaveDeviceRecord(grpc::ServerContext *context,
                                const SaveDeviceRecordRequest *request,
                                GenericResponse *response) override;

  grpc::Status CompareRecords(grpc::ServerContext *context,
                              const CompareRecordsRequest *request,
                              CompareRecordsResponse *response) override;

private:
  DeviceManager *manager_;
};

} // namespace device
