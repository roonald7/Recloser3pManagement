#include "RecloserServiceImpl.hpp"
#include <iostream>
#include <sstream>

namespace recloser {

RecloserServiceImpl::RecloserServiceImpl(RecloserManager *manager)
    : manager_(manager) {}

grpc::Status
RecloserServiceImpl::GetServiceTree(grpc::ServerContext *context,
                                    const ServiceTreeRequest *request,
                                    ServiceTreeResponse *response) {

  int firmwareId = request->firmware_id();

  std::cout << "GetServiceTree called for firmware_id=" << firmwareId
            << std::endl;

  // Get top-level services (parent_id = 0)
  auto topLevelServices =
      manager_->getServicesByParentAndFirmware(0, firmwareId);

  for (const auto &service : topLevelServices) {
    ServiceNode *node = response->add_top_level_services();
    node->set_id(service.id);
    node->set_service_key(service.service_key);

    auto translations =
        manager_->getTranslationsForKey(service.description_key);
    for (const auto &t : translations) {
      auto *trans = node->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    // Get features for this service
    auto features = manager_->getFeaturesByService(service.id);
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

    // Recursively build children
    buildServiceNode(service.id, firmwareId, node);
  }

  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::CompareServiceTrees(
    grpc::ServerContext *context, const CompareServiceTreesRequest *request,
    CompareServiceTreesResponse *response) {

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
  compareNodes(tree1, tree2, response->mutable_differences(), added, removed,
               modified);

  // Generate summary
  std::ostringstream summary;
  summary << added << " service(s) added, " << removed
          << " service(s) removed, " << modified << " service(s) modified";
  response->set_summary(summary.str());

  return grpc::Status::OK;
}

void RecloserServiceImpl::buildServiceNode(int parentId, int firmwareId,
                                           ServiceNode *parentNode) {

  auto childServices =
      manager_->getServicesByParentAndFirmware(parentId, firmwareId);

  for (const auto &service : childServices) {
    ServiceNode *childNode = parentNode->add_children();
    childNode->set_id(service.id);
    childNode->set_service_key(service.service_key);

    auto translations =
        manager_->getTranslationsForKey(service.description_key);
    for (const auto &t : translations) {
      auto *trans = childNode->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    // Get features for this child service
    auto features = manager_->getFeaturesByService(service.id);
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

    // Recursively build grandchildren
    buildServiceNode(service.id, firmwareId, childNode);
  }
}

void RecloserServiceImpl::buildInternalTree(
    int parentId, int firmwareId, const std::string &languageCode,
    std::map<std::string, ServiceTreeNode> &tree) {

  auto services =
      manager_->getServicesByParentAndFirmware(parentId, firmwareId);

  for (const auto &service : services) {
    ServiceTreeNode node;
    node.service_key = service.service_key;
    node.display_name =
        manager_->getTranslation(service.description_key, languageCode);

    // Get features
    auto features = manager_->getFeaturesByService(service.id);
    for (const auto &feat : features) {
      node.features.insert(feat.description_key);
    }

    // Recursively build children
    buildInternalTree(service.id, firmwareId, languageCode, node.children);

    tree[service.service_key] = node;
  }
}

void RecloserServiceImpl::compareNodes(
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
      diff->set_service_key(node1.service_key);
      diff->set_display_name(node1.display_name);
      diff->set_difference_type(DifferenceType::REMOVED);
      removed++;
    } else {
      // Service exists in both, check for modifications
      const auto &node2 = it2->second;
      bool hasChanges = false;

      ServiceDifference *diff = differences->Add();
      diff->set_service_key(node1.service_key);
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
      diff->set_service_key(node2.service_key);
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
RecloserServiceImpl::GetScreenLayout(grpc::ServerContext *context,
                                     const ScreenLayoutRequest *request,
                                     ScreenLayoutResponse *response) {

  int serviceId = request->service_id();

  std::cout << "GetScreenLayout called for service_id=" << serviceId
            << std::endl;

  auto layoutResult = manager_->getScreenLayout(serviceId);

  if (layoutResult) {
    populateServiceLayout(*layoutResult, response->mutable_service_layout());
    return grpc::Status::OK;
  } else {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Service or Layout not found");
  }
}

void RecloserServiceImpl::populateServiceLayout(
    const RecloserManager::ServiceLayoutRecord &rec, ServiceLayout *layout) {

  layout->set_service_id(rec.service_id);
  layout->set_service_key(rec.service_key);

  for (const auto &t : rec.translations) {
    auto *trans = layout->add_translations();
    trans->set_language_code(t.language_code);
    trans->set_value(t.value);
  }

  for (const auto &feat : rec.features) {
    FeatureLayoutDetail *detail = layout->add_features();
    detail->set_feature_id(feat.feature_id);
    detail->set_feature_key(feat.feature_key);

    for (const auto &t : feat.translations) {
      auto *trans = detail->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    detail->set_component_type(feat.component_type);
    detail->set_component_key(feat.component_key);

    for (const auto &lim : feat.limits) {
      LayoutLimit *limit = detail->add_limits();
      limit->set_key(lim.key);
      limit->set_value(lim.value);
    }
  }

  for (const auto &childRec : rec.children) {
    ServiceLayout *childLayout = layout->add_children();
    populateServiceLayout(childRec, childLayout);
  }
}

grpc::Status RecloserServiceImpl::CreateRecloser(grpc::ServerContext *context,
                                                 const RecloserRecord *request,
                                                 GenericResponse *response) {
  bool success =
      manager_->addRecloser(request->description_key(), request->model());
  response->set_success(success);
  response->set_message(success ? "Recloser created"
                                : "Failed to create recloser");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::UpdateRecloser(grpc::ServerContext *context,
                                                 const RecloserRecord *request,
                                                 GenericResponse *response) {
  bool success = manager_->updateRecloser(
      request->id(), request->description_key(), request->model());
  response->set_success(success);
  response->set_message(success ? "Recloser updated"
                                : "Failed to update recloser");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::DeleteRecloser(grpc::ServerContext *context,
                                                 const DeleteRequest *request,
                                                 GenericResponse *response) {
  bool success = manager_->deleteRecloser(request->id());
  response->set_success(success);
  response->set_message(success ? "Recloser deleted"
                                : "Failed to delete recloser");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::CreateFirmware(grpc::ServerContext *context,
                                                 const FirmwareRecord *request,
                                                 GenericResponse *response) {
  bool success =
      manager_->addFirmwareVersion(request->version(), request->recloser_id());
  response->set_success(success);
  response->set_message(success ? "Firmware created"
                                : "Failed to create firmware");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::UpdateFirmware(grpc::ServerContext *context,
                                                 const FirmwareRecord *request,
                                                 GenericResponse *response) {
  bool success = manager_->updateFirmwareVersion(
      request->id(), request->version(), request->recloser_id());
  response->set_success(success);
  response->set_message(success ? "Firmware updated"
                                : "Failed to update firmware");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::DeleteFirmware(grpc::ServerContext *context,
                                                 const DeleteRequest *request,
                                                 GenericResponse *response) {
  bool success = manager_->deleteFirmwareVersion(request->id());
  response->set_success(success);
  response->set_message(success ? "Firmware deleted"
                                : "Failed to delete firmware");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::AddServiceNode(grpc::ServerContext *context,
                                                 const ServiceRecord *request,
                                                 GenericResponse *response) {
  bool success =
      manager_->addService(request->service_key(), request->description_key(),
                           request->firmware_id(), request->parent_id());
  response->set_success(success);
  response->set_message(success ? "Service created"
                                : "Failed to create service");
  return grpc::Status::OK;
}

grpc::Status
RecloserServiceImpl::UpdateServiceNode(grpc::ServerContext *context,
                                       const ServiceRecord *request,
                                       GenericResponse *response) {
  bool success = manager_->updateService(
      request->id(), request->service_key(), request->description_key(),
      request->firmware_id(), request->parent_id());
  response->set_success(success);
  response->set_message(success ? "Service updated"
                                : "Failed to update service");
  return grpc::Status::OK;
}

grpc::Status
RecloserServiceImpl::DeleteServiceNode(grpc::ServerContext *context,
                                       const DeleteRequest *request,
                                       GenericResponse *response) {
  bool success = manager_->deleteService(request->id());
  response->set_success(success);
  response->set_message(success ? "Service deleted"
                                : "Failed to delete service");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::CreateFeature(grpc::ServerContext *context,
                                                const FeatureRecord *request,
                                                GenericResponse *response) {
  bool success =
      manager_->addFeature(request->description_key(), request->service_id());
  response->set_success(success);
  response->set_message(success ? "Feature created"
                                : "Failed to create feature");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::UpdateFeature(grpc::ServerContext *context,
                                                const FeatureRecord *request,
                                                GenericResponse *response) {
  bool success = manager_->updateFeature(
      request->id(), request->description_key(), request->service_id());
  response->set_success(success);
  response->set_message(success ? "Feature updated"
                                : "Failed to update feature");
  return grpc::Status::OK;
}

grpc::Status RecloserServiceImpl::DeleteFeature(grpc::ServerContext *context,
                                                const DeleteRequest *request,
                                                GenericResponse *response) {
  bool success = manager_->deleteFeature(request->id());
  response->set_success(success);
  response->set_message(success ? "Feature deleted"
                                : "Failed to delete feature");
  return grpc::Status::OK;
}

grpc::Status
RecloserServiceImpl::GetFullInventory(grpc::ServerContext *context,
                                      const FullInventoryRequest *request,
                                      FullInventoryResponse *response) {

  std::cout << "GetFullInventory called" << std::endl;

  auto reclosers = manager_->getAllReclosers();
  for (const auto &r : reclosers) {
    auto *ri = response->add_reclosers();
    ri->set_id(r.id);
    ri->set_model(r.model);

    auto rTranslations = manager_->getTranslationsForKey(r.description_key);
    for (const auto &t : rTranslations) {
      auto *trans = ri->add_translations();
      trans->set_language_code(t.language_code);
      trans->set_value(t.value);
    }

    auto firmwares = manager_->getFirmwareVersionsForRecloser(r.id);
    for (const auto &f : firmwares) {
      auto *fi = ri->add_firmwares();
      fi->set_id(f.id);
      fi->set_version(f.version);

      // Fetch top level services for this firmware
      auto topServices = manager_->getServicesByParentAndFirmware(0, f.id);
      for (const auto &s : topServices) {
        auto *sn = fi->add_services();
        sn->set_id(s.id);
        sn->set_service_key(s.service_key);

        auto sTranslations = manager_->getTranslationsForKey(s.description_key);
        for (const auto &t : sTranslations) {
          auto *trans = sn->add_translations();
          trans->set_language_code(t.language_code);
          trans->set_value(t.value);
        }

        // Get features for this top service
        auto features = manager_->getFeaturesByService(s.id);
        for (const auto &feat : features) {
          Feature *feature = sn->add_features();
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

        // Recursively build children
        buildServiceNode(s.id, f.id, sn);
      }
    }
  }

  return grpc::Status::OK;
}

} // namespace recloser
