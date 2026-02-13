#include "DeviceServiceImpl.hpp"
#include "ServiceHelpers.hpp"
#include <iostream>
#include <sstream>

namespace device {

DeviceServiceImpl::DeviceServiceImpl(DeviceManager *manager)
    : manager_(manager) {}

grpc::Status
DeviceServiceImpl::GetServiceTree(grpc::ServerContext *context,
                                  const ServiceTreeRequest *request,
                                  ServiceTreeResponse *response) {

  int deviceId = request->device_id();
  int modelId = request->model_id();
  int firmwareId = request->firmware_id();

  std::cout << "GetServiceTree called for device=" << deviceId
            << ", model=" << modelId << ", firmware=" << firmwareId
            << std::endl;

  auto tree =
      ServiceHelpers::getServiceTree(manager_, deviceId, modelId, firmwareId);

  for (const auto &nodeInfo : tree) {
    ServiceNode *node = response->add_top_level_services();
    fillServiceNode(nodeInfo, node);
  }

  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::GetLanguages(grpc::ServerContext *context,
                                             const LanguagesRequest *request,
                                             LanguagesResponse *response) {

  std::cout << "GetLanguages called" << std::endl;

  auto languages = manager_->getAllLanguages();
  for (const auto &lang : languages) {
    auto *l = response->add_languages();
    l->set_code(lang.code);
    l->set_name(lang.name);
  }

  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::CompareServiceTrees(
    grpc::ServerContext *context, const CompareServiceTreesRequest *request,
    CompareServiceTreesResponse *response) {

  int dmfId1 = request->device_model_firmware_id_1();
  int dmfId2 = request->device_model_firmware_id_2();
  std::string languageCode = request->language_code();

  std::cout << "CompareServiceTrees called for device_model_firmware_id_1="
            << dmfId1 << ", device_model_firmware_id_2=" << dmfId2
            << ", language=" << languageCode << std::endl;

  response->set_device_model_firmware_id_1(dmfId1);
  response->set_device_model_firmware_id_2(dmfId2);

  // Build internal tree structures for both firmwares
  std::map<std::string, ServiceTreeNode> tree1, tree2;
  buildInternalTree(0, dmfId1, languageCode, tree1);
  buildInternalTree(0, dmfId2, languageCode, tree2);

  // Compare the trees
  int added = 0, removed = 0, modified = 0;
  compareNodes(tree1, tree2, response->mutable_differences(), added, removed,
               modified);

  // Generate summary
  std::ostringstream summary;
  summary << added << " service(s) added, " << removed
          << " service(s) removed, " << modified << " service(s) modified";
  response->set_summary(summary.str());

  return grpc::Status::OK;
}

void DeviceServiceImpl::fillServiceNode(const ServiceNodeInfo &info,
                                        ServiceNode *node) {
  node->set_id(info.id);
  node->set_description_key(info.description_key);

  for (const auto &t : info.translations) {
    auto *trans = node->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  for (const auto &feat : info.features) {
    Feature *feature = node->add_features();
    feature->set_id(feat.id);
    feature->set_feature_key(feat.feature_key);

    for (const auto &t : feat.translations) {
      auto *trans = feature->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }
  }

  for (const auto &childInfo : info.children) {
    ServiceNode *childNode = node->add_children();
    fillServiceNode(childInfo, childNode);
  }
}

void DeviceServiceImpl::buildServiceNode(int parentId, int mfId,
                                         ServiceNode *parentNode) {

  auto childServices =
      manager_->getServicesByParentAndModelFirmware(parentId, mfId);

  for (const auto &service : childServices) {
    ServiceNode *childNode = parentNode->add_children();
    childNode->set_id(service.id);
    childNode->set_description_key(service.description_key);

    auto translations =
        manager_->getTranslationsForKey(service.description_key);
    for (const auto &t : translations) {
      auto *trans = childNode->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    // Get screen features for this child service-firmware combination
    int smfId = manager_->getServiceModelFirmwareId(service.id, mfId);
    if (smfId > 0) {
      auto screenFeatures =
          manager_->getScreenFeaturesByServiceModelFirmware(smfId);
      for (const auto &sf : screenFeatures) {
        Feature *feature = childNode->add_features();
        feature->set_id(sf.feature_id); // Base feature ID
        feature->set_feature_key(sf.description_key);

        auto fTranslations =
            manager_->getTranslationsForKey(sf.description_key);
        for (const auto &t : fTranslations) {
          auto *trans = feature->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }
      }
    }
    // Recursively build grandchildren
    buildServiceNode(service.id, mfId, childNode);
  }
}

void DeviceServiceImpl::buildInternalTree(
    int parentId, int dmfId, const std::string &languageCode,
    std::map<std::string, ServiceTreeNode> &tree) {

  auto services =
      manager_->getServicesByParentAndModelFirmware(parentId, dmfId);

  for (const auto &service : services) {
    ServiceTreeNode node;
    node.description_key = service.description_key;
    node.display_name =
        manager_->getTranslation(service.description_key, languageCode);

    // Get screen features for this service-firmware combination
    int smfId = manager_->getServiceModelFirmwareId(service.id, dmfId);
    if (smfId > 0) {
      auto screenFeatures =
          manager_->getScreenFeaturesByServiceModelFirmware(smfId);
      for (const auto &sf : screenFeatures) {
        node.features.insert(sf.description_key);
      }
    }

    // Recursively build children
    buildInternalTree(service.id, dmfId, languageCode, node.children);

    tree[service.description_key] = node;
  }
}

void DeviceServiceImpl::compareNodes(
    const std::map<std::string, ServiceTreeNode> &tree1,
    const std::map<std::string, ServiceTreeNode> &tree2,
    google::protobuf::RepeatedPtrField<ServiceDifference> *differences,
    int &added, int &removed, int &modified) {

  // Find services in tree1 (check for removed or modified)
  for (const auto &[key, node1] : tree1) {
    auto it2 = tree2.find(key);

    if (it2 == tree2.end()) {
      // Service removed in tree2
      ServiceDifference *diff = differences->Add();
      diff->set_description_key(node1.description_key);
      diff->set_display_name(node1.display_name);
      diff->set_difference_type(DifferenceType::REMOVED);
      removed++;
    } else {
      // Service exists in both, check for modifications
      const auto &node2 = it2->second;
      bool hasChanges = false;

      ServiceDifference *diff = differences->Add();
      diff->set_description_key(node1.description_key);
      diff->set_display_name(node1.display_name);

      // Compare features
      for (const auto &feat : node1.features) {
        if (node2.features.find(feat) == node2.features.end()) {
          FeatureDifference *featDiff = diff->add_feature_differences();
          featDiff->set_feature_name(feat);
          featDiff->set_difference_type(DifferenceType::REMOVED);
          hasChanges = true;
        }
      }

      for (const auto &feat : node2.features) {
        if (node1.features.find(feat) == node1.features.end()) {
          FeatureDifference *featDiff = diff->add_feature_differences();
          featDiff->set_feature_name(feat);
          featDiff->set_difference_type(DifferenceType::ADDED);
          hasChanges = true;
        }
      }

      // Recursively compare children
      int childAdded = 0, childRemoved = 0, childModified = 0;
      compareNodes(node1.children, node2.children,
                   diff->mutable_child_differences(), childAdded, childRemoved,
                   childModified);

      if (hasChanges || childAdded > 0 || childRemoved > 0 ||
          childModified > 0) {
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
  for (const auto &[key, node2] : tree2) {
    if (tree1.find(key) == tree1.end()) {
      ServiceDifference *diff = differences->Add();
      diff->set_description_key(node2.description_key);
      diff->set_display_name(node2.display_name);
      diff->set_difference_type(DifferenceType::ADDED);

      // Add all features as new
      for (const auto &feat : node2.features) {
        FeatureDifference *featDiff = diff->add_feature_differences();
        featDiff->set_feature_name(feat);
        featDiff->set_difference_type(DifferenceType::ADDED);
      }

      added++;
    }
  }
}

grpc::Status
DeviceServiceImpl::GetScreenLayout(grpc::ServerContext *context,
                                   const ScreenLayoutRequest *request,
                                   ScreenLayoutResponse *response) {

  int deviceId = request->device_id();
  int modelId = request->model_id();
  int firmwareId = request->firmware_id();
  int serviceId = request->service_id();
  int64_t physicalDeviceId = request->physical_device_id();

  std::cout << "GetScreenLayout called for device_id=" << deviceId
            << ", model_id=" << modelId << ", firmware_id=" << firmwareId
            << ", service_id=" << serviceId
            << ", physical_device_id=" << physicalDeviceId << std::endl;

  // Get the device_model_firmware_id directly from model and firmware
  int dmfId = manager_->getModelFirmwareId(modelId, firmwareId);
  if (dmfId <= 0) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Model-Firmware combination not found");
  }

  int targetServiceId = serviceId;
  if (targetServiceId <= 0) {
    // If no serviceId is provided, try to find the first root service
    auto topServices = manager_->getServicesByParentAndModelFirmware(0, dmfId);
    if (!topServices.empty()) {
      targetServiceId = topServices[0].id;
    }
  }

  if (targetServiceId <= 0) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "No services found for this firmware");
  }

  // Load saved values if a physical device is specified
  std::map<int, std::string> savedValues;
  if (physicalDeviceId > 0) {
    auto values = manager_->getValuesForPhysicalDevice(physicalDeviceId);
    for (const auto &v : values) {
      savedValues[v.feature_id] = v.value;
    }
  }

  // Get the service-model-firmware id for the layout
  int smfId = manager_->getServiceModelFirmwareId(targetServiceId, dmfId);
  if (smfId <= 0) {
    // Attempt to link it if it doesn't exist, as it's required for features
    smfId = manager_->linkServiceToModelFirmware(targetServiceId, dmfId);
  }

  if (smfId <= 0) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Service not linked to this firmware");
  }

  auto layoutResult = manager_->getScreenLayout(smfId);

  if (layoutResult) {
    populateServiceLayout(*layoutResult, response->mutable_service_layout(),
                          savedValues);
    return grpc::Status::OK;
  } else {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Service or Layout not found");
  }
}

void DeviceServiceImpl::populateServiceLayout(
    const DeviceManager::ServiceLayoutRecord &rec, ServiceLayout *layout,
    const std::map<int, std::string> &savedValues) {

  layout->set_service_id(rec.service_id);
  layout->set_description_key(rec.description_key);

  for (const auto &t : rec.translations) {
    auto *trans = layout->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  for (const auto &feat : rec.features) {
    FeatureComponentDetail *detail = layout->add_features();
    populateFeatureDetail(feat, detail, savedValues);
  }

  for (const auto &childRec : rec.children) {
    ServiceLayout *childLayout = layout->add_children();
    populateServiceLayout(childRec, childLayout, savedValues);
  }
}

void DeviceServiceImpl::populateFeatureDetail(
    const DeviceManager::FeatureComponentRecord &rec,
    FeatureComponentDetail *detail,
    const std::map<int, std::string> &savedValues) {
  detail->set_feature_id(rec.feature_id);
  detail->set_feature_key(rec.feature_key);

  for (const auto &t : rec.translations) {
    auto *trans = detail->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  detail->set_component_type(rec.component_type);

  for (const auto &lim : rec.limits) {
    ComponentLimit *limit = detail->add_limits();
    limit->set_key(lim.key);
    limit->set_value(lim.value);
  }

  // Set saved value if available
  if (savedValues.count(rec.feature_id)) {
    detail->set_value(savedValues.at(rec.feature_id));
  }

  // Add all options (includes ON/OFF for toggles and regular options for
  // comboboxes)
  for (const auto &opt : rec.options) {
    auto *option = detail->add_options();
    option->set_value(opt.value);
    for (const auto &t : opt.translations) {
      auto *trans = option->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }
  }

  for (const auto &child : rec.children) {
    FeatureComponentDetail *childDetail = detail->add_children();
    populateFeatureDetail(child, childDetail, savedValues);
  }
}

grpc::Status DeviceServiceImpl::CreateDevice(grpc::ServerContext *context,
                                             const DeviceRecord *request,
                                             GenericResponse *response) {
  bool success = manager_->addDevice(request->description_key());
  response->set_success(success);
  response->set_message(success ? "Device created" : "Failed to create device");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::UpdateDevice(grpc::ServerContext *context,
                                             const DeviceRecord *request,
                                             GenericResponse *response) {
  bool success =
      manager_->updateDevice(request->id(), request->description_key());
  response->set_success(success);
  response->set_message(success ? "Device updated" : "Failed to update device");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::DeleteDevice(grpc::ServerContext *context,
                                             const DeleteRequest *request,
                                             GenericResponse *response) {
  bool success = manager_->deleteDevice(request->id());
  response->set_success(success);
  response->set_message(success ? "Device deleted" : "Failed to delete device");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::CreateFirmware(grpc::ServerContext *context,
                                               const FirmwareRecord *request,
                                               GenericResponse *response) {
  int firmwareId = manager_->addFirmwareVersion(request->description_key());
  bool success = (firmwareId > 0);
  if (success && request->model_id() > 0) {
    int dmfId = manager_->linkFirmwareToModel(firmwareId, request->model_id());
    success = (dmfId > 0);
  }
  response->set_success(success);
  response->set_message(success ? "Firmware created"
                                : "Failed to create firmware");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::UpdateFirmware(grpc::ServerContext *context,
                                               const FirmwareRecord *request,
                                               GenericResponse *response) {
  // Description_key is unique, so "update" is essentially ensure exists and
  // link
  int firmwareId = manager_->addFirmwareVersion(request->description_key());
  bool success = (firmwareId > 0);
  if (success && request->model_id() > 0) {
    int dmfId = manager_->linkFirmwareToModel(firmwareId, request->model_id());
    success = (dmfId > 0);
  }
  response->set_success(success);
  response->set_message(success ? "Firmware updated"
                                : "Failed to update firmware");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::DeleteFirmware(grpc::ServerContext *context,
                                               const DeleteRequest *request,
                                               GenericResponse *response) {
  bool success = manager_->deleteFirmwareVersion(request->id());
  response->set_success(success);
  response->set_message(success ? "Firmware deleted"
                                : "Failed to delete firmware");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::AddServiceNode(grpc::ServerContext *context,
                                               const ServiceRecord *request,
                                               GenericResponse *response) {
  int serviceId =
      manager_->addService(request->description_key(), request->parent_id());
  bool success = (serviceId > 0);

  response->set_success(success);
  response->set_message(success ? "Service created"
                                : "Failed to create service");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::UpdateServiceNode(grpc::ServerContext *context,
                                                  const ServiceRecord *request,
                                                  GenericResponse *response) {
  bool success = manager_->updateService(
      request->id(), request->description_key(), request->parent_id());
  response->set_success(success);
  response->set_message(success ? "Service updated"
                                : "Failed to update service");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::DeleteServiceNode(grpc::ServerContext *context,
                                                  const DeleteRequest *request,
                                                  GenericResponse *response) {
  bool success = manager_->deleteService(request->id());
  response->set_success(success);
  response->set_message(success ? "Service deleted"
                                : "Failed to delete service");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::CreateFeature(grpc::ServerContext *context,
                                              const FeatureRecord *request,
                                              GenericResponse *response) {
  int featureId = manager_->addFeature(request->key());
  bool success = (featureId > 0);
  response->set_success(success);
  response->set_message(success ? "Feature created"
                                : "Failed to create feature");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::UpdateFeature(grpc::ServerContext *context,
                                              const FeatureRecord *request,
                                              GenericResponse *response) {
  bool success = manager_->updateFeature(request->id(), request->key());
  response->set_success(success);
  response->set_message(success ? "Feature updated"
                                : "Failed to update feature");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::DeleteFeature(grpc::ServerContext *context,
                                              const DeleteRequest *request,
                                              GenericResponse *response) {
  bool success = manager_->deleteFeature(request->id());
  response->set_success(success);
  response->set_message(success ? "Feature deleted"
                                : "Failed to delete feature");
  return grpc::Status::OK;
}

grpc::Status
DeviceServiceImpl::CreateScreenFeature(grpc::ServerContext *context,
                                       const ScreenFeatureRecord *request,
                                       GenericResponse *response) {
  int id = manager_->addScreenFeature(
      request->service_model_firmware_id(), request->feature_id(),
      request->description_key(), request->parent_screen_feature_id());
  bool success = (id > 0);
  response->set_success(success);
  response->set_message(success ? "Screen Feature created"
                                : "Failed to create screen feature");
  return grpc::Status::OK;
}

grpc::Status
DeviceServiceImpl::UpdateScreenFeature(grpc::ServerContext *context,
                                       const ScreenFeatureRecord *request,
                                       GenericResponse *response) {
  bool success = manager_->updateScreenFeature(
      request->id(), request->service_model_firmware_id(),
      request->feature_id(), request->description_key(),
      request->parent_screen_feature_id());
  response->set_success(success);
  response->set_message(success ? "Screen Feature updated"
                                : "Failed to update screen feature");
  return grpc::Status::OK;
}

grpc::Status
DeviceServiceImpl::DeleteScreenFeature(grpc::ServerContext *context,
                                       const DeleteRequest *request,
                                       GenericResponse *response) {
  bool success = manager_->deleteScreenFeature(request->id());
  response->set_success(success);
  response->set_message(success ? "Screen Feature deleted"
                                : "Failed to delete screen feature");
  return grpc::Status::OK;
}

grpc::Status
DeviceServiceImpl::GetFullInventory(grpc::ServerContext *context,
                                    const FullInventoryRequest *request,
                                    FullInventoryResponse *response) {

  std::cout << "GetFullInventory called" << std::endl;

  auto devices = manager_->getAllDevices();
  for (const auto &d : devices) {
    auto *di = response->add_devices();
    di->set_id(d.id);
    di->set_description_key(d.description_key);

    auto dTranslations = manager_->getTranslationsForKey(d.description_key);
    for (const auto &t : dTranslations) {
      auto *trans = di->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    auto models = manager_->getModelsForDevice(d.id);
    for (const auto &model : models) {
      auto *mi = di->add_models();
      mi->set_id(model.id);
      mi->set_description_key(model.description_key);

      auto mTranslations =
          manager_->getTranslationsForKey(model.description_key);
      for (const auto &t : mTranslations) {
        auto *trans = mi->add_translations();
        trans->set_language_code(t.language_code);
        trans->set_value(t.value);
      }

      auto firmwares = manager_->getFirmwareVersionsForModel(model.id);
      for (const auto &f : firmwares) {
        auto *fi = mi->add_firmwares();
        int mfId = manager_->getModelFirmwareId(model.id, f.id);
        fi->set_id(mfId); // Returning DMF ID as the "firmware id" for inventory
        fi->set_description_key(f.description_key);

        auto fTranslations = manager_->getTranslationsForKey(f.description_key);
        for (const auto &t : fTranslations) {
          auto *trans = fi->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }

        // Fetch top level services for this model-firmware combination
        auto topServices =
            manager_->getServicesByParentAndModelFirmware(0, mfId);
        for (const auto &s : topServices) {
          auto *sn = fi->add_services();
          sn->set_id(s.id);
          sn->set_description_key(s.description_key);

          auto sTranslations =
              manager_->getTranslationsForKey(s.description_key);
          for (const auto &t : sTranslations) {
            auto *trans = sn->add_translations();
            trans->set_language_code(t.language_code);
            trans->set_value(t.value);
          }

          int smfId = manager_->getServiceModelFirmwareId(s.id, mfId);
          if (smfId > 0) {
            auto screenFeatures =
                manager_->getScreenFeaturesByServiceModelFirmware(smfId);
            for (const auto &sf : screenFeatures) {
              Feature *feature = sn->add_features();
              feature->set_id(sf.feature_id);
              feature->set_feature_key(sf.description_key);

              auto ftTranslations =
                  manager_->getTranslationsForKey(sf.description_key);
              for (const auto &t : ftTranslations) {
                auto *trans = feature->add_translations();
                trans->set_language_code(t.language_code);
                trans->set_value(t.value);
              }
            }
          }
          buildServiceNode(s.id, mfId, sn);
        }
      }
    }
  }

  return grpc::Status::OK;
}

grpc::Status
DeviceServiceImpl::GetDeviceInformation(grpc::ServerContext *context,
                                        const DeviceInformationRequest *request,
                                        DeviceInformationResponse *response) {
  auto deviceInfos = ServiceHelpers::getDeviceInformation(manager_);
  for (const auto &di : deviceInfos) {
    auto *deviceProto = response->add_devices();
    deviceProto->set_id(di.device.id);
    deviceProto->set_description_key(di.device.description_key);
    for (const auto &t : di.translations) {
      auto *trans = deviceProto->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }
    for (const auto &mi : di.models) {
      auto *modelProto = deviceProto->add_models();
      modelProto->set_id(mi.model.id);
      modelProto->set_description_key(mi.model.description_key);
      for (const auto &t : mi.translations) {
        auto *trans = modelProto->add_translations();
        trans->set_language_code(t.language_code);
        trans->set_value(t.value);
      }
      for (const auto &fi : mi.firmwares) {
        auto *fwProto = modelProto->add_firmwares();
        fwProto->set_id(fi.firmware.id);
        fwProto->set_description_key(fi.firmware.description_key);
        for (const auto &t : fi.translations) {
          auto *trans = fwProto->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }
      }
    }
  }
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::ComparePhysicalDevices(
    grpc::ServerContext *context, const ComparePhysicalDevicesRequest *request,
    ComparePhysicalDevicesResponse *response) {

  int64_t id1 = request->physical_device_id_1();
  int64_t id2 = request->physical_device_id_2();
  std::string languageCode = request->language_code();

  std::cout << "ComparePhysicalDevices (Hierarchical) called for id1=" << id1
            << ", id2=" << id2 << ", language=" << languageCode << std::endl;

  auto dev1 = manager_->getPhysicalDeviceById(id1);
  auto dev2 = manager_->getPhysicalDeviceById(id2);

  if (!dev1 || !dev2) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "One or both physical devices not found");
  }

  response->set_physical_device_id_1(id1);
  response->set_physical_device_id_2(id2);

  // Use Device 1's firmware as the structural base
  int dmfId =
      manager_->getModelFirmwareId(dev1->model_id, dev1->firmware_version_id);

  auto tree = ServiceHelpers::getServiceTree(
      manager_, dev1->device_id, dev1->model_id, dev1->firmware_version_id);

  // Load values for comparison
  auto values1 = manager_->getValuesForPhysicalDevice(id1);
  auto values2 = manager_->getValuesForPhysicalDevice(id2);

  std::map<int, std::string> valMap1, valMap2;
  // Note: For different firmwares, we would need to map by key,
  // but for now we assume similar structures or same firmware.
  for (const auto &v : values1)
    valMap1[v.feature_id] = v.value;
  for (const auto &v : values2)
    valMap2[v.feature_id] = v.value;

  for (const auto &nodeInfo : tree) {
    auto *rootNode = response->add_root_services();
    buildPhysicalComparisonNode(nodeInfo, id1, id2, valMap1, valMap2, rootNode);
  }

  return grpc::Status::OK;
}

bool DeviceServiceImpl::buildPhysicalComparisonNode(
    const ServiceNodeInfo &info, int64_t id1, int64_t id2,
    const std::map<int, std::string> &valMap1,
    const std::map<int, std::string> &valMap2,
    PhysicalDeviceServiceComparison *node) {

  node->set_service_id(info.id);
  node->set_description_key(info.description_key);
  bool hasDiff = false;

  for (const auto &t : info.translations) {
    auto *trans = node->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  // Features comparison
  for (const auto &feat : info.features) {
    auto *featComp = node->add_features();
    featComp->set_feature_id(feat.id);
    featComp->set_feature_key(feat.feature_key);

    for (const auto &t : feat.translations) {
      auto *trans = featComp->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    std::string v1 = valMap1.count(feat.id) ? valMap1.at(feat.id) : "";
    std::string v2 = valMap2.count(feat.id) ? valMap2.at(feat.id) : "";

    featComp->set_value_1(v1);
    featComp->set_value_2(v2);
    bool isDiff = (v1 != v2);
    featComp->set_is_different(isDiff);

    if (isDiff)
      hasDiff = true;
  }

  // Recursive children
  for (const auto &childInfo : info.children) {
    auto *childNode = node->add_children();
    if (buildPhysicalComparisonNode(childInfo, id1, id2, valMap1, valMap2,
                                    childNode)) {
      hasDiff = true;
    }
  }

  node->set_has_differences(hasDiff);
  return hasDiff;
}

grpc::Status DeviceServiceImpl::GetAllPhysicalDevices(
    grpc::ServerContext *context, const GetAllPhysicalDevicesRequest *request,
    GetAllPhysicalDevicesResponse *response) {

  std::cout << "GetAllPhysicalDevices called" << std::endl;

  auto devices = manager_->getAllPhysicalDevices();
  for (const auto &d : devices) {
    auto *pd = response->add_physical_devices();
    pd->set_id(d.id);
    pd->set_name(d.name);
    pd->set_device_id(d.device_id);
    pd->set_model_id(d.model_id);
    pd->set_firmware_version_id(d.firmware_version_id);
    pd->set_identifier(d.identifier);
    pd->set_description(d.description);
    pd->set_comment(d.comment);
    pd->set_is_template(d.is_template);
  }

  return grpc::Status::OK;
}

RecordServiceImpl::RecordServiceImpl(DeviceManager *manager)
    : manager_(manager) {}

grpc::Status
RecordServiceImpl::GetAllRecords(grpc::ServerContext *context,
                                 const GetAllRecordsRequest *request,
                                 GetAllRecordsResponse *response) {
  std::cout << "GetAllRecords called" << std::endl;

  auto devices = manager_->getAllPhysicalDevices();
  for (const auto &d : devices) {
    auto *rec = response->add_records();
    rec->set_id(d.id);
    rec->set_name(d.name);
    rec->set_identifier(d.identifier);

    // Get names for device, model, firmware
    auto dev = manager_->getDeviceById(d.device_id);
    if (dev)
      rec->set_device_name(manager_->getTranslation(dev->description_key,
                                                    request->language_code()));

    auto mod = manager_->getModelById(d.model_id);
    if (mod)
      rec->set_model_name(manager_->getTranslation(mod->description_key,
                                                   request->language_code()));

    auto fw = manager_->getFirmwareVersionById(d.firmware_version_id);
    if (fw)
      rec->set_firmware_name(manager_->getTranslation(
          fw->description_key, request->language_code()));

    // Get all values
    auto values = manager_->getValuesForPhysicalDevice(d.id);
    for (const auto &v : values) {
      auto *val = rec->add_values();
      val->set_feature_id(v.feature_id);
      val->set_value(v.value);

      auto feat = manager_->getFeatureById(v.feature_id);
      if (feat) {
        val->set_feature_key(feat->key);
        auto translations = manager_->getTranslationsForKey(feat->key);
        for (const auto &t : translations) {
          auto *trans = val->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }
      }
    }
  }
  return grpc::Status::OK;
}

} // namespace device
