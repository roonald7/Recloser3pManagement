#include "DeviceManager.hpp"
#include "DeviceServiceImpl.hpp"
#include <filesystem>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <vector>

void RunServer(DeviceManager *manager, const std::string &server_address) {
  device::DeviceServiceImpl service(manager);
  device::RecordServiceImpl recordService(manager);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.RegisterService(&recordService);

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
  manager->addKeyWithTranslations("ZEUS_NG_3P4W",
                                  {{"enUs", "3P/4W"}, {"ptBr", "3P/4W"}});
  manager->addKeyWithTranslations("ZEUS_NG_1P2W",
                                  {{"enUs", "1P/2W"}, {"ptBr", "1P/2W"}});

  // Add Devices
  int dZeusNg = manager->addDevice("ZEUS_NG"); // ID 1

  // Add Models
  int m3p4w = manager->addModel("ZEUS_NG_3P4W", dZeusNg); // ID 1
  int m1p2w = manager->addModel("ZEUS_NG_1P2W", dZeusNg); // ID 2

  // Add Firmware Versions
  manager->addKeyWithTranslations("ZEUS_NG_V_1_0_0",
                                  {{"enUs", "v1.0.0"}, {"ptBr", "v1.0.0"}});
  manager->addKeyWithTranslations("ZEUS_NG_V_2_0_0",
                                  {{"enUs", "v2.0.0"}, {"ptBr", "v2.0.0"}});
  manager->addKeyWithTranslations("ZEUS_NG_V_1_1_0",
                                  {{"enUs", "v1.1.0"}, {"ptBr", "v1.1.0"}});
  manager->addKeyWithTranslations("ZEUS_NG_v3.0.0",
                                  {{"enUs", "v3.0.0"}, {"ptBr", "v3.0.0"}});

  // For Zeus NG 3P/4W
  int fwV1 = manager->addFirmwareVersion("ZEUS_NG_V_1_0_0");
  int fwV2 = manager->addFirmwareVersion("ZEUS_NG_V_2_0_0");
  int dmfV1M1 = manager->linkFirmwareToModel(fwV1, m3p4w);
  int dmfV2M1 = manager->linkFirmwareToModel(fwV2, m3p4w);

  // For Zeus NG 1P/2W
  int fwV11 = manager->addFirmwareVersion("ZEUS_NG_V_1_1_0");
  int dmfV11M2 = manager->linkFirmwareToModel(fwV11, m1p2w);

  // Setup Services and Sections (Hierarchy)

  // 1. Setup Shared Services (Reusable across firmwares)
  int sDateTime = manager->addService("DATE_TIME", 0);
  int sMultiplicationConstants =
      manager->addService("MULTIPLICATION_CONSTANTS", 0);

  // Link Services to ModelFirmware
  int smfV1DateTime = manager->linkServiceToModelFirmware(sDateTime, dmfV1M1);
  int smfV1MultConst =
      manager->linkServiceToModelFirmware(sMultiplicationConstants, dmfV1M1);

  // Add Features to Model 1 / Firmware v1
  int fDateV1 = manager->addFeature("DATE", smfV1DateTime);
  int fTimeV1 = manager->addFeature("TIME", smfV1DateTime);
  manager->linkFeatureToComponent(fDateV1, "Date");
  manager->linkFeatureToComponent(fTimeV1, "Time");

  // Add Features to Model 1 / Firmware v1 using sMultiplicationConstants link
  int fNumTcV1 = manager->addFeature("NUM_TC", smfV1MultConst);
  int fDenTcV1 = manager->addFeature("DEN_TC", smfV1MultConst);
  int fNumTpV1 = manager->addFeature("NUM_TP", smfV1MultConst);
  int fDenTpV1 = manager->addFeature("DEN_TP", smfV1MultConst);

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

  // For V2:
  int smfV2DateTime = manager->linkServiceToModelFirmware(sDateTime, dmfV2M1);
  int smfV2MultConst =
      manager->linkServiceToModelFirmware(sMultiplicationConstants, dmfV2M1);

  // Add Features to Model 1 / Firmware v2
  int fDateV2 = manager->addFeature("DATE", smfV2DateTime);
  int fTimeV2 = manager->addFeature("TIME", smfV2DateTime);
  int fGmtV2 = manager->addFeature("GMT", smfV2DateTime); // Only v2 has gmt !

  manager->linkFeatureToComponent(fDateV2, "Date");
  manager->linkFeatureToComponent(fTimeV2, "Time");
  int fcGmtV2 = manager->linkFeatureToComponent(fGmtV2, "Spinner");

  manager->addComponentLimit(fcGmtV2, "MIN_VALUE", "-12");
  manager->addComponentLimit(fcGmtV2, "MAX_VALUE", "12");
  manager->addComponentLimit(fcGmtV2, "DEFAULT_VALUE", "0");
  manager->addComponentLimit(fcGmtV2, "STEP", "1");

  int fTcWindow = manager->addFeature("TC_WINDOW", smfV2MultConst);
  int fTpWindow = manager->addFeature("TP_WINDOW", smfV2MultConst);
  int fNumTcV2 = manager->addFeature("NUM_TC", smfV2MultConst, fTcWindow);
  int fDenTcV2 = manager->addFeature("DEN_TC", smfV2MultConst, fTcWindow);
  int fNumTpV2 = manager->addFeature("NUM_TP", smfV2MultConst, fTpWindow);
  int fDenTpV2 = manager->addFeature("DEN_TP", smfV2MultConst, fTpWindow);

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
  int fwV3 = manager->addFirmwareVersion("ZEUS_NG_v3.0.0");
  int dmfV3M1 = manager->linkFirmwareToModel(fwV3, 1);
  int sAdvanced =
      manager->addService("ADVANCED_SETTINGS", 0); // Top levelService

  int smfV3Adv = manager->linkServiceToModelFirmware(sAdvanced, dmfV3M1);

  // Create a Window Feature
  int fWindow = manager->addFeature("SETTINGS_WINDOW", smfV3Adv);
  manager->linkFeatureToComponent(fWindow, "Window");

  // Add child features to the Window (using new parent_feature_id arg)
  int fWinDate = manager->addFeature("WIN_DATE", smfV3Adv, fWindow);
  int fWinTime = manager->addFeature("WIN_TIME", smfV3Adv, fWindow);

  manager->linkFeatureToComponent(fWinDate, "Date");
  manager->linkFeatureToComponent(fWinTime, "Time");

  // Create Zeus V1 Physical Device
  PhysicalDeviceRecord zeusV2_a;
  zeusV2_a.name = "Zeus V2 Unit A";
  zeusV2_a.device_id = 1; // Zeus NG
  zeusV2_a.model_id = 1;  // 3P/4W
  zeusV2_a.firmware_version_id = fwV2;
  zeusV2_a.identifier = "SN-ZV2-001";
  zeusV2_a.description = "Zeus Unit running Firmware V2";
  zeusV2_a.is_template = false;
  int64_t zid1 = manager->addPhysicalDevice(zeusV2_a);
  if (zid1 > 0) {
    manager->setPhysicalDeviceValue(zid1, fDateV2, "2023-01-01");
    manager->setPhysicalDeviceValue(zid1, fTimeV2, "12:00:00");
    manager->setPhysicalDeviceValue(zid1, fGmtV2, "1");
  }

  // Create Zeus V2 Physical Device
  PhysicalDeviceRecord zeusV2_b;
  zeusV2_b.name = "Zeus V2 Unit B";
  zeusV2_b.device_id = 1; // Zeus NG
  zeusV2_b.model_id = 1;  // 3P/4W
  zeusV2_b.firmware_version_id = fwV2;
  zeusV2_b.identifier = "SN-ZV2-002";
  zeusV2_b.description = "Zeus Unit running Firmware V2";
  zeusV2_b.is_template = false;
  int64_t zid2 = manager->addPhysicalDevice(zeusV2_b);
  if (zid2 > 0) {
    manager->setPhysicalDeviceValue(zid2, fDateV2, "2024-02-11");
    manager->setPhysicalDeviceValue(zid2, fTimeV2, "09:55:00");
    manager->setPhysicalDeviceValue(zid2, fGmtV2, "-3");
  }
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

  manager->addKeyWithTranslations("RECLOSER_3P_v1.0.0",
                                  {{"enUs", "v1.0.0"}, {"ptBr", "v1.0.0"}});
  // Add Devices
  int dRecloser3p = manager->addDevice("RECLOSER_3P"); // ID 1

  // Add Models
  int mModel3p4w = manager->addModel("RECLOSER_3P", dRecloser3p); // ID 1

  // Add Firmware Versions
  int fwV1R3P = manager->addFirmwareVersion("RECLOSER_3P_v1.0.0");
  int dmfV1R3P = manager->linkFirmwareToModel(fwV1R3P, mModel3p4w);
  // Setup Services and Sections (Hierarchy)

  // 1. Setup Shared Services (Reusable across firmwares)
  int sProtectionParameters =
      manager->addService("SERV_PROTECTION_PARAMETERS", 0);
  int sBasicProtectionConfig = manager->addService(
      "SERV_BASIC_PROTECTION_CONFIG", sProtectionParameters);
  int sOvervoltageStage1Config = manager->addService(
      "SERV_OVERVOLTAGE_STAGE_1_CONFIG", sProtectionParameters);

  // Link Services to ModelFirmware
  manager->linkServiceToModelFirmware(sProtectionParameters, dmfV1R3P);
  int smfBasProt =
      manager->linkServiceToModelFirmware(sBasicProtectionConfig, dmfV1R3P);
  int smfOvVstage1 =
      manager->linkServiceToModelFirmware(sOvervoltageStage1Config, dmfV1R3P);

  // Add Features to Model R3P / Firmware v1
  int fEnabledAutomaticResetV1 =
      manager->addFeature("ENABLED_AUTOMATIC_RESET", smfBasProt);
  int fAutomaticResetTimeV1 =
      manager->addFeature("AUTOMATIC_RESET_TIME", smfBasProt);
  int fcEnabledAutomaticResetV1 =
      manager->linkFeatureToComponent(fEnabledAutomaticResetV1, "Toggle");
  int fcAutomaticResetTimeV1 =
      manager->linkFeatureToComponent(fAutomaticResetTimeV1, "Time");

  manager->addComponentLimit(fcEnabledAutomaticResetV1, "ON_LABEL", "ENABLED");
  manager->addComponentLimit(fcEnabledAutomaticResetV1, "OFF_LABEL",
                             "DISABLED");
  manager->addComponentLimit(fcAutomaticResetTimeV1, "DEFAULT_VALUE", "5");

  // Add Features to Model R3P / Firmware v1
  int fEnabledOvervoltageStage1V1 =
      manager->addFeature("ENABLED_OVERVOLTAGE_STAGE_1", smfOvVstage1);
  int fOvervoltageStage1ThresholdV1 =
      manager->addFeature("OVERVOLTAGE_STAGE_1_THRESHOLD", smfOvVstage1);
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
  int fRecloseAttempts =
      manager->addFeature("AUTOMATIC_RECLOSE_ATTEMPTS", smfBasProt);
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

  // Generate 2 records for current configurations
  PhysicalDeviceRecord dev1;
  dev1.name = "Recloser Unit A";
  dev1.device_id = dRecloser3p;
  dev1.model_id = mModel3p4w;
  dev1.firmware_version_id = fwV1R3P;
  dev1.identifier = "SN-R3P-001";
  dev1.description = "Primary Recloser for Substation Alpha";
  dev1.comment = "Initial deployment";
  dev1.is_template = false;
  int64_t id1 = manager->addPhysicalDevice(dev1);

  if (id1 > 0) {
    manager->setPhysicalDeviceValue(id1, fEnabledAutomaticResetV1, "1");
    manager->setPhysicalDeviceValue(id1, fAutomaticResetTimeV1, "10");
    manager->setPhysicalDeviceValue(id1, fRecloseAttempts, "3");
  }

  PhysicalDeviceRecord dev2;
  dev2.name = "Standard Template";
  dev2.device_id = dRecloser3p;
  dev2.model_id = mModel3p4w;
  dev2.firmware_version_id = fwV1R3P;
  dev2.identifier = "TEMP-R3P-STD";
  dev2.description = "Standard configuration for rural feeders";
  dev2.comment = "Use this as base for new units";
  dev2.is_template = true;
  manager->addPhysicalDevice(dev2);
}
