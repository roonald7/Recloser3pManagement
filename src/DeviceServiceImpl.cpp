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

  int dmfId = request->device_model_firmware_id();

  std::cout << "GetServiceTree called for device_model_firmware_id=" << dmfId
            << std::endl;

  // Get top-level services (parent_id = 0)
  auto topLevelServices =
      manager_->getServicesByParentAndDeviceModelFirmware(0, dmfId);

  for (const auto &service : topLevelServices) {
    ServiceNode *node = response->add_top_level_services();
    int sdfId = manager_->getServiceDeviceModelFirmwareId(service.id, dmfId);
    node->set_id(sdfId);
    node->set_description_key(service.description_key);

    auto translations =
        manager_->getTranslationsForKey(service.description_key);
    for (const auto &t : translations) {
      auto *trans = node->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    // Get features for this service-firmware combination
    if (sdfId > 0) {
      auto features = manager_->getFeaturesByServiceDeviceModelFirmware(sdfId);
      for (const auto &feat : features) {
        Feature *feature = node->add_features();
        feature->set_id(feat.id);
        feature->set_feature_key(feat.description_key);

        auto fTranslations =
            manager_->getTranslationsForKey(feat.description_key);
        for (const auto &t : fTranslations) {
          auto *trans = feature->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }
      }
    }

    // Recursively build children
    buildServiceNode(service.id, dmfId, node);
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

void DeviceServiceImpl::buildServiceNode(int parentId, int dmfId,
                                         ServiceNode *parentNode) {

  auto childServices =
      manager_->getServicesByParentAndDeviceModelFirmware(parentId, dmfId);

  for (const auto &service : childServices) {
    ServiceNode *childNode = parentNode->add_children();
    int sdfId = manager_->getServiceDeviceModelFirmwareId(service.id, dmfId);
    childNode->set_id(sdfId);
    childNode->set_description_key(service.description_key);

    auto translations =
        manager_->getTranslationsForKey(service.description_key);
    for (const auto &t : translations) {
      auto *trans = childNode->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    // Get features for this child service-firmware combination
    if (sdfId > 0) {
      auto features = manager_->getFeaturesByServiceDeviceModelFirmware(sdfId);
      for (const auto &feat : features) {
        Feature *feature = childNode->add_features();
        feature->set_id(feat.id);
        feature->set_feature_key(feat.description_key);

        auto fTranslations =
            manager_->getTranslationsForKey(feat.description_key);
        for (const auto &t : fTranslations) {
          auto *trans = feature->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }
      }
    }

    // Recursively build grandchildren
    buildServiceNode(service.id, dmfId, childNode);
  }
}

void DeviceServiceImpl::buildInternalTree(
    int parentId, int dmfId, const std::string &languageCode,
    std::map<std::string, ServiceTreeNode> &tree) {

  auto services =
      manager_->getServicesByParentAndDeviceModelFirmware(parentId, dmfId);

  for (const auto &service : services) {
    ServiceTreeNode node;
    node.description_key = service.description_key;
    node.display_name =
        manager_->getTranslation(service.description_key, languageCode);

    // Get features for this service-firmware combination
    int sdfId = manager_->getServiceDeviceModelFirmwareId(service.id, dmfId);
    if (sdfId > 0) {
      auto features = manager_->getFeaturesByServiceDeviceModelFirmware(sdfId);
      for (const auto &feat : features) {
        node.features.insert(feat.description_key);
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

  int sdfId = request->service_device_model_firmware_id();

  std::cout << "GetScreenLayout called for service_device_model_firmware_id="
            << sdfId << std::endl;

  auto layoutResult = manager_->getScreenLayout(sdfId);

  if (layoutResult) {
    populateServiceLayout(*layoutResult, response->mutable_service_layout());
    return grpc::Status::OK;
  } else {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Service or Layout not found");
  }
}

void DeviceServiceImpl::populateServiceLayout(
    const DeviceManager::ServiceLayoutRecord &rec, ServiceLayout *layout) {

  layout->set_service_id(rec.service_id);
  layout->set_description_key(rec.description_key);

  for (const auto &t : rec.translations) {
    auto *trans = layout->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  for (const auto &feat : rec.features) {
    FeatureComponentDetail *detail = layout->add_features();
    populateFeatureDetail(feat, detail);
  }

  for (const auto &childRec : rec.children) {
    ServiceLayout *childLayout = layout->add_children();
    populateServiceLayout(childRec, childLayout);
  }
}

void DeviceServiceImpl::populateFeatureDetail(
    const DeviceManager::FeatureComponentRecord &rec,
    FeatureComponentDetail *detail) {
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

  for (const auto &t : rec.on_translations) {
    auto *trans = detail->add_on_label_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  for (const auto &t : rec.off_translations) {
    auto *trans = detail->add_off_label_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

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
    populateFeatureDetail(child, childDetail);
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
  if (success && request->device_model_id() > 0) {
    int dmfId =
        manager_->linkFirmwareToModel(firmwareId, request->device_model_id());
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
  if (success && request->device_model_id() > 0) {
    int dmfId =
        manager_->linkFirmwareToModel(firmwareId, request->device_model_id());
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
  if (success && request->device_model_firmware_id() > 0) {
    success = manager_->linkServiceToDeviceModelFirmware(
                  serviceId, request->device_model_firmware_id()) > 0;
  }

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
  bool success = manager_->addFeature(
      request->description_key(), request->service_device_model_firmware_id());
  response->set_success(success);
  response->set_message(success ? "Feature created"
                                : "Failed to create feature");
  return grpc::Status::OK;
}

grpc::Status DeviceServiceImpl::UpdateFeature(grpc::ServerContext *context,
                                              const FeatureRecord *request,
                                              GenericResponse *response) {
  bool success =
      manager_->updateFeature(request->id(), request->description_key(),
                              request->service_device_model_firmware_id());
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

    auto deviceModels = manager_->getDeviceModelsForDevice(d.id);
    for (const auto &dm : deviceModels) {
      auto modelOpt = manager_->getModelById(dm.model_id);
      if (!modelOpt)
        continue;

      auto *mi = di->add_models();
      mi->set_id(modelOpt->id);
      mi->set_device_model_id(dm.id);
      mi->set_description_key(modelOpt->description_key);

      auto mTranslations =
          manager_->getTranslationsForKey(modelOpt->description_key);
      for (const auto &t : mTranslations) {
        auto *trans = mi->add_translations();
        trans->set_language_code(t.language_code);
        trans->set_value(t.value);
      }

      auto firmwares = manager_->getFirmwareVersionsForDeviceModel(dm.id);
      for (const auto &f : firmwares) {
        auto *fi = mi->add_firmwares();
        int dmfId = manager_->getDeviceModelFirmwareId(dm.id, f.id);
        fi->set_id(
            dmfId); // Returning DMF ID as the "firmware id" for inventory
        fi->set_description_key(f.description_key);

        auto fTranslations = manager_->getTranslationsForKey(f.description_key);
        for (const auto &t : fTranslations) {
          auto *trans = fi->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }

        // Fetch top level services for this model-firmware combination
        auto topServices =
            manager_->getServicesByParentAndDeviceModelFirmware(0, dmfId);
        for (const auto &s : topServices) {
          auto *sn = fi->add_services();
          int sdfId = manager_->getServiceDeviceModelFirmwareId(s.id, dmfId);
          sn->set_id(sdfId);
          sn->set_description_key(s.description_key);

          auto sTranslations =
              manager_->getTranslationsForKey(s.description_key);
          for (const auto &t : sTranslations) {
            auto *trans = sn->add_translations();
            trans->set_language_code(t.language_code);
            trans->set_value(t.value);
          }

          if (sdfId > 0) {
            auto features =
                manager_->getFeaturesByServiceDeviceModelFirmware(sdfId);
            for (const auto &feat : features) {
              Feature *feature = sn->add_features();
              feature->set_id(feat.id);
              feature->set_feature_key(feat.description_key);

              auto ftTranslations =
                  manager_->getTranslationsForKey(feat.description_key);
              for (const auto &t : ftTranslations) {
                auto *trans = feature->add_translations();
                trans->set_language_code(t.language_code);
                trans->set_value(t.value);
              }
            }
          }
          buildServiceNode(s.id, dmfId, sn);
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

} // namespace device
