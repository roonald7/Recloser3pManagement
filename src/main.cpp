#include "DeviceManager.hpp"
#include "DeviceServiceImpl.hpp"
#include <filesystem>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <vector>

void RunServer(DeviceManager *manager, const std::string &server_address) {
  device::DeviceServiceImpl service(manager);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "gRPC Server listening on " << server_address << std::endl;

  server->Wait();
}

void initialize_zeus_ng(DeviceManager *manager);
void initialize_recloser_3p(DeviceManager *manager);

int main() {
  std::cout << "--- 3P Device Management System ---" << std::endl;

  // Ensure data directory exists
  std::filesystem::create_directories("data");

  // Use a clearer name for the management database
  DeviceManager manager("data/management.db");

  if (!manager.initialize()) {
    std::cerr << "Failed to initialize database." << std::endl;
    return 1;
  }

  std::cout << "Database initialized at: data/management.db" << std::endl;

  if (manager.getAllDevices().empty()) {
    std::cout << "Database is empty. Populating initial data..." << std::endl;

    // Setup Languages
    manager.addLanguage("enUs", "English");
    manager.addLanguage("ptBr", "Português");

    initialize_zeus_ng(&manager);
    initialize_recloser_3p(&manager);
  }

  // Start gRPC server in a separate thread
  std::cout << "\n--- Starting gRPC Server ---" << std::endl;
  std::string server_address("0.0.0.0:50051");

  std::thread server_thread(RunServer, &manager, server_address);

  std::cout << "\nPress Ctrl+C to stop the server..." << std::endl;

  server_thread.join();

  return 0;
}

void initialize_zeus_ng(DeviceManager *manager) {
  // Setup Description Keys and Translations
  manager->addKeyWithTranslations(
      "DATE_TIME", {{"enUs", "Date and Time"}, {"ptBr", "Data e Hora"}});
  manager->addKeyWithTranslations("DATE", {{"enUs", "Date"}, {"ptBr", "Data"}});
  manager->addKeyWithTranslations("TIME", {{"enUs", "Time"}, {"ptBr", "Hora"}});
  manager->addKeyWithTranslations("GMT", {{"enUs", "GMT"}, {"ptBr", "GMT"}});

  manager->addKeyWithTranslations("MULTIPLICATION_CONSTANTS",
                                  {{"enUs", "Multiplication Constants"},
                                   {"ptBr", "Constantes de Multiplicação"}});
  manager->addKeyWithTranslations(
      "NUM_TC", {{"enUs", "TC numerator"}, {"ptBr", "Numerador do TC"}});
  manager->addKeyWithTranslations(
      "DEN_TC", {{"enUs", "TC denominator"}, {"ptBr", "Denominador do TC"}});
  manager->addKeyWithTranslations(
      "NUM_TP", {{"enUs", "TP numerator"}, {"ptBr", "Numerador do TP"}});
  manager->addKeyWithTranslations(
      "DEN_TP", {{"enUs", "TP denominator"}, {"ptBr", "Denominador do TP"}});

  manager->addKeyWithTranslations(
      "TC_WINDOW", {{"enUs", "TC Settings"}, {"ptBr", "Configurações TC"}});
  manager->addKeyWithTranslations(
      "TP_WINDOW", {{"enUs", "TP Settings"}, {"ptBr", "Configurações TP"}});

  // Setup Description Keys for Models
  manager->addKeyWithTranslations("ZEUS_NG",
                                  {{"enUs", "ZEUS NG"}, {"ptBr", "ZEUS NG"}});
  manager->addKeyWithTranslations("MODEL_3P4W",
                                  {{"enUs", "3P/4W"}, {"ptBr", "3P/4W"}});
  manager->addKeyWithTranslations("MODEL_1P2W",
                                  {{"enUs", "1P/2W"}, {"ptBr", "1P/2W"}});

  // Add Devices
  manager->addDevice("ZEUS_NG"); // ID 1

  // Add Models
  manager->addModel("MODEL_3P4W"); // ID 1
  manager->addModel("MODEL_1P2W"); // ID 2

  // Link Devices to Models
  manager->addDeviceModel(1, 1); // Zeus NG + 3P/4W -> DeviceModel ID 1
  manager->addDeviceModel(1, 2); // Zeus NG + 1P/2W -> DeviceModel ID 2

  // Add Firmware Versions
  // For Zeus NG 3P/4W (DeviceModel ID 1)
  manager->addFirmwareVersion("v1.0.0", 1);
  manager->addFirmwareVersion("v2.0.0", 1);

  // For Zeus NG 1P/2W (DeviceModel ID 2)
  manager->addFirmwareVersion("v1.1.0", 2);

  // Setup Services and Sections (Hierarchy)

  // 1. Setup Shared Services (Reusable across firmwares)
  int sDateTime = manager->addService("DATE_TIME", 0);
  int sMultiplicationConstants =
      manager->addService("MULTIPLICATION_CONSTANTS", 0);

  // 2. Link Services to Firmware 1 (v1.0.0)
  int sfDateTimeV1 = manager->linkServiceToFirmware(sDateTime, 1);
  int sfMultiplicationConstantsV1 =
      manager->linkServiceToFirmware(sMultiplicationConstants, 1);

  // 3. Link Same Services to Firmware 2 (v2.1.2)
  int sfDateTimeV2 = manager->linkServiceToFirmware(sDateTime, 2);
  int sfMultiplicationConstantsV2 =
      manager->linkServiceToFirmware(sMultiplicationConstants, 2);

  // Add Features to Firmware v1 (Link to sfDateTimeV1)
  int fDateV1 = manager->addFeature("DATE", sfDateTimeV1);
  int fTimeV1 = manager->addFeature("TIME", sfDateTimeV1);
  manager->linkFeatureToComponent(fDateV1, "Date");
  manager->linkFeatureToComponent(fTimeV1, "Time");

  // Add Features to Firmware v1 (Link to sfMultiplicationConstantsV1)
  int fNumTcV1 = manager->addFeature("NUM_TC", sfMultiplicationConstantsV1);
  int fDenTcV1 = manager->addFeature("DEN_TC", sfMultiplicationConstantsV1);
  int fNumTpV1 = manager->addFeature("NUM_TP", sfMultiplicationConstantsV1);
  int fDenTpV1 = manager->addFeature("DEN_TP", sfMultiplicationConstantsV1);

  int fcNmTcV1 = manager->linkFeatureToComponent(fNumTcV1, "Integer");
  int fcDnTcV1 = manager->linkFeatureToComponent(fDenTcV1, "Integer");
  int fcNmTpV1 = manager->linkFeatureToComponent(fNumTpV1, "Integer");
  int fcDnTpV1 = manager->linkFeatureToComponent(fDenTpV1, "Integer");

  manager->addComponentLimit(fcNmTcV1, "MIN_VALUE", "1");
  manager->addComponentLimit(fcNmTcV1, "MAX_VALUE", "10000");
  manager->addComponentLimit(fcDnTcV1, "MIN_VALUE", "1");
  manager->addComponentLimit(fcDnTcV1, "MAX_VALUE", "10000");
  manager->addComponentLimit(fcNmTpV1, "MIN_VALUE", "1");
  manager->addComponentLimit(fcNmTpV1, "MAX_VALUE", "10000");
  manager->addComponentLimit(fcDnTpV1, "MIN_VALUE", "1");
  manager->addComponentLimit(fcDnTpV1, "MAX_VALUE", "10000");
  manager->addComponentLimit(fcNmTcV1, "DEFAULT_VALUE", "5");
  manager->addComponentLimit(fcNmTcV1, "STEP", "5");

  // Add Features to Firmware v2 (Link to sfParamsV2)
  int fDateV2 = manager->addFeature("DATE", sfDateTimeV2);
  int fTimeV2 = manager->addFeature("TIME", sfDateTimeV2);
  int fGmtV2 = manager->addFeature("GMT", sfDateTimeV2); // Only v2 has gmt !

  manager->linkFeatureToComponent(fDateV2, "Date");
  manager->linkFeatureToComponent(fTimeV2, "Time");
  int fcGmtV2 = manager->linkFeatureToComponent(fGmtV2, "Spinner");

  manager->addComponentLimit(fcGmtV2, "MIN_VALUE", "-12");
  manager->addComponentLimit(fcGmtV2, "MAX_VALUE", "12");
  manager->addComponentLimit(fcGmtV2, "DEFAULT_VALUE", "0");
  manager->addComponentLimit(fcGmtV2, "STEP", "1");

  int fTcWindow = manager->addFeature("TC_WINDOW", sfMultiplicationConstantsV2);
  int fTpWindow = manager->addFeature("TP_WINDOW", sfMultiplicationConstantsV2);
  int fNumTcV2 =
      manager->addFeature("NUM_TC", sfMultiplicationConstantsV2, fTcWindow);
  int fDenTcV2 =
      manager->addFeature("DEN_TC", sfMultiplicationConstantsV2, fTcWindow);
  int fNumTpV2 =
      manager->addFeature("NUM_TP", sfMultiplicationConstantsV2, fTpWindow);
  int fDenTpV2 =
      manager->addFeature("DEN_TP", sfMultiplicationConstantsV2, fTpWindow);

  manager->linkFeatureToComponent(fTcWindow, "Window");
  manager->linkFeatureToComponent(fTpWindow, "Window");
  int fcNmTcV2 = manager->linkFeatureToComponent(fNumTcV2, "Integer");
  int fcDnTcV2 = manager->linkFeatureToComponent(fDenTcV2, "Integer");
  int fcNmTpV2 = manager->linkFeatureToComponent(fNumTpV2, "Integer");
  int fcDnTpV2 = manager->linkFeatureToComponent(fDenTpV2, "Integer");

  manager->addComponentLimit(fcNmTcV2, "MIN_VALUE", "1");
  manager->addComponentLimit(fcNmTcV2, "MAX_VALUE", "20000");
  manager->addComponentLimit(fcDnTcV2, "MIN_VALUE", "1");
  manager->addComponentLimit(fcDnTcV2, "MAX_VALUE", "20000");
  manager->addComponentLimit(fcNmTpV2, "MIN_VALUE", "1");
  manager->addComponentLimit(fcNmTpV2, "MAX_VALUE", "20000");
  manager->addComponentLimit(fcDnTpV2, "MIN_VALUE", "1");
  manager->addComponentLimit(fcDnTpV2, "MAX_VALUE", "10000");

  // Example of Container / Window (New Firmware v3 for demonstration)
  manager->addFirmwareVersion("v3.0.0", 1);
  int sAdvanced =
      manager->addService("ADVANCED_SETTINGS", 0); // Top levelService
  int sfAdvancedV3 = manager->linkServiceToFirmware(sAdvanced, 5);

  // Create a Window Feature
  int fWindow = manager->addFeature("SETTINGS_WINDOW", sfAdvancedV3);
  manager->linkFeatureToComponent(fWindow, "Window");

  // Add child features to the Window (using new parent_feature_id arg)
  int fWinDate = manager->addFeature("WIN_DATE", sfAdvancedV3, fWindow);
  int fWinTime = manager->addFeature("WIN_TIME", sfAdvancedV3, fWindow);

  manager->linkFeatureToComponent(fWinDate, "Date");
  manager->linkFeatureToComponent(fWinTime, "Time");
}

void initialize_recloser_3p(DeviceManager *manager) {
  // Setup Description Keys and Translations
  manager->addKeyWithTranslations(
      "RECLOSER_3P", {{"enUs", "Recloser 3P"}, {"ptBr", "Recloser 3P"}});

  manager->addKeyWithTranslations(
      "SERV_PROTECTION_PARAMETERS",
      {{"enUs", "Protection Parameters"}, {"ptBr", "Parametros de Proteção"}});

  manager->addKeyWithTranslations(
      "SERV_BASIC_PROTECTION_CONFIG",
      {{"enUs", "Basic Protection Config"},
       {"ptBr", "Configuração Básica de Proteção"}});
  manager->addKeyWithTranslations(
      "SERV_OVERVOLTAGE_STAGE_1_CONFIG",
      {{"enUs", "Overvoltage Stage 1 Config"},
       {"ptBr", "Configuração Sobretensão Estágio 1"}});

  manager->addKeyWithTranslations(
      "ENABLED", {{"enUs", "Enabled"}, {"ptBr", "Habilitado"}});
  manager->addKeyWithTranslations(
      "DISABLED", {{"enUs", "Disabled"}, {"ptBr", "Desabilitado"}});

  manager->addKeyWithTranslations("ENABLED_AUTOMATIC_RESET",
                                  {{"enUs", "Enabled Automatic Reset"},
                                   {"ptBr", "Habilitar Reset Automático"}});
  manager->addKeyWithTranslations("AUTOMATIC_RESET_TIME",
                                  {{"enUs", "Automatic Reset Time"},
                                   {"ptBr", "Tempo de Reset Automático"}});
  manager->addKeyWithTranslations(
      "AUTOMATIC_RECLOSE_ATTEMPTS",
      {{"enUs", "Automatic Reclose Attempts"},
       {"ptBr", "Tentativas de Releitura Automática"}});

  manager->addKeyWithTranslations(
      "ENABLED_OVERVOLTAGE_STAGE_1",
      {{"enUs", "Enabled Overvoltage Stage 1"},
       {"ptBr", "Habilitar Sobretensão Estágio 1"}});
  manager->addKeyWithTranslations(
      "OVERVOLTAGE_STAGE_1_THRESHOLD",
      {{"enUs", "Overvoltage Stage 1 Threshold"},
       {"ptBr", "Limite de Sobretensão Estágio 1"}});

  manager->addKeyWithTranslations(
      "ENABLED", {{"enUs", "Enabled"}, {"ptBr", "Habilitado"}});
  manager->addKeyWithTranslations(
      "DISABLED", {{"enUs", "Disabled"}, {"ptBr", "Desabilitado"}});

  // Add Devices
  int dRecloser3p = manager->addDevice("RECLOSER_3P"); // ID 1

  // Add Models
  int mModel3p4w = manager->addModel("RECLOSER_3P"); // ID 1

  // Link Devices to Models
  int dmRecloser3p =
      manager->addDeviceModel(dRecloser3p, mModel3p4w); // DeviceModel table

  // Add Firmware Versions
  // For Recloser 3P (DeviceModel ID 1)
  int fwRecloser3p = manager->addFirmwareVersion("v1.0.0", dmRecloser3p);
  // Setup Services and Sections (Hierarchy)

  // 1. Setup Shared Services (Reusable across firmwares)
  int sProtectionParameters =
      manager->addService("SERV_PROTECTION_PARAMETERS", 0);
  int sBasicProtectionConfig = manager->addService(
      "SERV_BASIC_PROTECTION_CONFIG", sProtectionParameters);
  int sOvervoltageStage1Config = manager->addService(
      "SERV_OVERVOLTAGE_STAGE_1_CONFIG", sProtectionParameters);

  // 2. Link Services to Firmware 1 (v1.0.0)
  int sfProtectionParametersV1 =
      manager->linkServiceToFirmware(sProtectionParameters, fwRecloser3p);
  int sfBasicProtectionConfigV1 =
      manager->linkServiceToFirmware(sBasicProtectionConfig, fwRecloser3p);
  int sfOvervoltageStage1ConfigV1 =
      manager->linkServiceToFirmware(sOvervoltageStage1Config, fwRecloser3p);

  // Add Features to Firmware v1 (Link to sfBasicProtectionConfigV1)
  int fEnabledAutomaticResetV1 =
      manager->addFeature("ENABLED_AUTOMATIC_RESET", sfBasicProtectionConfigV1);
  int fAutomaticResetTimeV1 =
      manager->addFeature("AUTOMATIC_RESET_TIME", sfBasicProtectionConfigV1);
  int fcEnabledAutomaticResetV1 =
      manager->linkFeatureToComponent(fEnabledAutomaticResetV1, "Toggle");
  int fcAutomaticResetTimeV1 =
      manager->linkFeatureToComponent(fAutomaticResetTimeV1, "Time");

  manager->addComponentLimit(fcEnabledAutomaticResetV1, "ON_LABEL", "ENABLED");
  manager->addComponentLimit(fcEnabledAutomaticResetV1, "OFF_LABEL",
                             "DISABLED");
  manager->addComponentLimit(fcAutomaticResetTimeV1, "DEFAULT_VALUE", "5");

  // Add Features to Firmware v1 (Link to sfOvervoltageStage1ConfigV1)
  int fEnabledOvervoltageStage1V1 = manager->addFeature(
      "ENABLED_OVERVOLTAGE_STAGE_1", sfOvervoltageStage1ConfigV1);
  int fOvervoltageStage1ThresholdV1 = manager->addFeature(
      "OVERVOLTAGE_STAGE_1_THRESHOLD", sfOvervoltageStage1ConfigV1);
  int fcEnabledOvervoltageStage1V1 =
      manager->linkFeatureToComponent(fEnabledOvervoltageStage1V1, "Toggle");
  int fcOvervoltageStage1ThresholdV1 =
      manager->linkFeatureToComponent(fOvervoltageStage1ThresholdV1, "Time");

  manager->addComponentLimit(fcEnabledOvervoltageStage1V1, "ON_LABEL",
                             "ENABLED");
  manager->addComponentLimit(fcEnabledOvervoltageStage1V1, "OFF_LABEL",
                             "DISABLED");
  manager->addComponentLimit(fcOvervoltageStage1ThresholdV1, "DEFAULT_VALUE",
                             "5");

  // Add a ComboBox example: Automatic Reclose Attempts
  int fRecloseAttempts = manager->addFeature("AUTOMATIC_RECLOSE_ATTEMPTS",
                                             sfBasicProtectionConfigV1);
  int fcRecloseAttempts =
      manager->linkFeatureToComponent(fRecloseAttempts, "ComboBox");

  manager->addKeyWithTranslations(
      "OPT_1_ATTEMPT", {{"enUs", "1 Attempt"}, {"ptBr", "1 Tentativa"}});
  manager->addKeyWithTranslations(
      "OPT_2_ATTEMPTS", {{"enUs", "2 Attempts"}, {"ptBr", "2 Tentativas"}});
  manager->addKeyWithTranslations(
      "OPT_3_ATTEMPTS", {{"enUs", "3 Attempts"}, {"ptBr", "3 Tentativas"}});

  manager->addComponentOption(fcRecloseAttempts, "1", "OPT_1_ATTEMPT");
  manager->addComponentOption(fcRecloseAttempts, "2", "OPT_2_ATTEMPTS");
  manager->addComponentOption(fcRecloseAttempts, "3", "OPT_3_ATTEMPTS");
}