#include "RecloserManager.hpp"
#include "RecloserServiceImpl.hpp"
#include <filesystem>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <vector>

void RunServer(RecloserManager *manager, const std::string &server_address) {
  recloser::RecloserServiceImpl service(manager);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  server->Wait();
}

int main() {
  std::cout << "--- 3P Recloser Management System ---" << std::endl;

  // Ensure data directory exists
  std::filesystem::create_directories("data");

  // Use a clearer name for the management database
  RecloserManager manager("data/management.db");

  if (!manager.initialize()) {
    std::cerr << "Failed to initialize database." << std::endl;
    return 1;
  }

  std::cout << "Database initialized at: data/management.db" << std::endl;

  if (manager.getAllReclosers().empty()) {
    std::cout << "Database is empty. Populating initial data..." << std::endl;

    // Setup Languages
    manager.addLanguage("enUs", "English");
    manager.addLanguage("ptBr", "Português");

    // Setup Description Keys and Translations
    manager.addKeyWithTranslations(
        "DATE_TIME", {{"enUs", "Date and Time"}, {"ptBr", "Data e Hora"}});
    manager.addKeyWithTranslations("DATE",
                                   {{"enUs", "Date"}, {"ptBr", "Data"}});
    manager.addKeyWithTranslations("TIME",
                                   {{"enUs", "Time"}, {"ptBr", "Hora"}});
    manager.addKeyWithTranslations("GMT", {{"enUs", "GMT"}, {"ptBr", "GMT"}});

    manager.addKeyWithTranslations("MULTIPLICATION_CONSTANTS",
                                   {{"enUs", "Multiplication Constants"},
                                    {"ptBr", "Constantes de Multiplicação"}});
    manager.addKeyWithTranslations(
        "NUM_TC", {{"enUs", "TC numerator"}, {"ptBr", "Numerador do TC"}});
    manager.addKeyWithTranslations(
        "DEN_TC", {{"enUs", "TC denominator"}, {"ptBr", "Denominador do TC"}});
    manager.addKeyWithTranslations(
        "NUM_TP", {{"enUs", "TP numerator"}, {"ptBr", "Numerador do TP"}});
    manager.addKeyWithTranslations(
        "DEN_TP", {{"enUs", "TP denominator"}, {"ptBr", "Denominador do TP"}});

    // Setup Description Keys for Reclosers
    manager.addKeyWithTranslations(
        "ZEUS_NG_3P4W", {{"enUs", "Zeus NG 3P/4W"}, {"ptBr", "Zeus NG 3P/4W"}});
    manager.addKeyWithTranslations(
        "ZEUS_NG_1P2W", {{"enUs", "Zeus NG 1P/2W"}, {"ptBr", "Zeus NG 1P/2W"}});
    manager.addKeyWithTranslations("ZEUS_NG",
                                   {{"enUs", "Zeus NG"}, {"ptBr", "Zeus NG"}});

    // Add Reclosers
    manager.addRecloser("ZEUS_NG_3P4W");
    manager.addRecloser("ZEUS_NG_1P2W");

    // Add Firmware Versions
    // For Model 1 (ID 1)
    manager.addFirmwareVersion("v1.0.0", 1);
    manager.addFirmwareVersion("v2.0.0", 1);
    // For Model 2 (ID 2)
    manager.addFirmwareVersion("v1.1.0", 2);
    manager.addFirmwareVersion("v2.1.0", 2);

    // Setup Services and Sections (Hierarchy)

    // 1. Setup Shared Services (Reusable across firmwares)
    int sDateTime = manager.addService("DATE_TIME", 0);
    int sMultiplicationConstants =
        manager.addService("MULTIPLICATION_CONSTANTS", 0);

    // 2. Link Services to Firmware 1 (v1.0.0)
    int sfDateTimeV1 = manager.linkServiceToFirmware(sDateTime, 1);
    int sfMultiplicationConstantsV1 =
        manager.linkServiceToFirmware(sMultiplicationConstants, 1);

    // 3. Link Same Services to Firmware 2 (v2.1.2)
    int sfDateTimeV2 = manager.linkServiceToFirmware(sDateTime, 2);
    int sfMultiplicationConstantsV2 =
        manager.linkServiceToFirmware(sMultiplicationConstants, 2);

    // Add Features to Firmware v1 (Link to sfDateTimeV1)
    int fDateV1 = manager.addFeature("DATE", sfDateTimeV1);
    int fTimeV1 = manager.addFeature("TIME", sfDateTimeV1);
    manager.linkFeatureToComponent(fDateV1, "Date");
    manager.linkFeatureToComponent(fTimeV1, "Time");

    // Add Features to Firmware v1 (Link to sfMultiplicationConstantsV1)
    int fNumTcV1 = manager.addFeature("NUM_TC", sfMultiplicationConstantsV1);
    int fDenTcV1 = manager.addFeature("DEN_TC", sfMultiplicationConstantsV1);
    int fNumTpV1 = manager.addFeature("NUM_TP", sfMultiplicationConstantsV1);
    int fDenTpV1 = manager.addFeature("DEN_TP", sfMultiplicationConstantsV1);

    int fcNmTcV1 = manager.linkFeatureToComponent(fNumTcV1, "Integer");
    int fcDnTcV1 = manager.linkFeatureToComponent(fDenTcV1, "Integer");
    int fcNmTpV1 = manager.linkFeatureToComponent(fNumTpV1, "Integer");
    int fcDnTpV1 = manager.linkFeatureToComponent(fDenTpV1, "Integer");

    manager.addComponentLimit(fcNmTcV1, "MIN_VALUE", "1");
    manager.addComponentLimit(fcNmTcV1, "MAX_VALUE", "10000");
    manager.addComponentLimit(fcDnTcV1, "MIN_VALUE", "1");
    manager.addComponentLimit(fcDnTcV1, "MAX_VALUE", "10000");
    manager.addComponentLimit(fcNmTpV1, "MIN_VALUE", "1");
    manager.addComponentLimit(fcNmTpV1, "MAX_VALUE", "10000");
    manager.addComponentLimit(fcDnTpV1, "MIN_VALUE", "1");
    manager.addComponentLimit(fcDnTpV1, "MAX_VALUE", "10000");
    manager.addComponentLimit(fcNmTcV1, "DEFAULT_VALUE", "5");
    manager.addComponentLimit(fcNmTcV1, "STEP", "5");

    // Add Features to Firmware v2 (Link to sfParamsV2)
    int fDateV2 = manager.addFeature("DATE", sfDateTimeV2);
    int fTimeV2 = manager.addFeature("TIME", sfDateTimeV2);
    int fGmtV2 = manager.addFeature("GMT", sfDateTimeV2); // Only v2 has gmt !

    manager.linkFeatureToComponent(fDateV2, "Date");
    manager.linkFeatureToComponent(fTimeV2, "Time");
    manager.linkFeatureToComponent(fGmtV2, "Spinner");

    int fNumTcV2 = manager.addFeature("NUM_TC", sfMultiplicationConstantsV2);
    int fDenTcV2 = manager.addFeature("DEN_TC", sfMultiplicationConstantsV2);
    int fNumTpV2 = manager.addFeature("NUM_TP", sfMultiplicationConstantsV2);
    int fDenTpV2 = manager.addFeature("DEN_TP", sfMultiplicationConstantsV2);

    int fcNmTcV2 = manager.linkFeatureToComponent(fNumTcV2, "Integer");
    int fcDnTcV2 = manager.linkFeatureToComponent(fDenTcV2, "Integer");
    int fcNmTpV2 = manager.linkFeatureToComponent(fNumTpV2, "Integer");
    int fcDnTpV2 = manager.linkFeatureToComponent(fDenTpV2, "Integer");

    manager.addComponentLimit(fcNmTcV2, "MIN_VALUE", "1");
    manager.addComponentLimit(fcNmTcV2, "MAX_VALUE", "20000");
    manager.addComponentLimit(fcDnTcV2, "MIN_VALUE", "1");
    manager.addComponentLimit(fcDnTcV2, "MAX_VALUE", "20000");
    manager.addComponentLimit(fcNmTpV2, "MIN_VALUE", "1");
    manager.addComponentLimit(fcNmTpV2, "MAX_VALUE", "20000");
    manager.addComponentLimit(fcDnTpV2, "MIN_VALUE", "1");
    manager.addComponentLimit(fcDnTpV2, "MAX_VALUE", "10000");
  }

  // Start gRPC server in a separate thread
  std::cout << "\n--- Starting gRPC Server ---" << std::endl;
  std::string server_address("0.0.0.0:50051");

  std::thread server_thread(RunServer, &manager, server_address);

  std::cout << "\nPress Ctrl+C to stop the server..." << std::endl;

  server_thread.join();

  return 0;
}
