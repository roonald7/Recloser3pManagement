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

  if (manager.getAllLines().empty()) {
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

  // Add Lines
  int dZeusNg = manager->addLine("ZEUS_NG"); // ID 1

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

  // Define Generic Features
  int fDate = manager->addFeature("DATE");
  int fTime = manager->addFeature("TIME");
  int fGmt = manager->addFeature("GMT");
  int fNumTc = manager->addFeature("NUM_TC");
  int fDenTc = manager->addFeature("DEN_TC");
  int fNumTp = manager->addFeature("NUM_TP");
  int fDenTp = manager->addFeature("DEN_TP");

  // Add Features to Model 1 / Firmware v1
  int sfDateV1 = manager->addScreenFeature(smfV1DateTime, fDate, "DATE");
  int sfTimeV1 = manager->addScreenFeature(smfV1DateTime, fTime, "TIME");
  manager->linkScreenFeatureToComponent(sfDateV1, "Date");
  manager->linkScreenFeatureToComponent(sfTimeV1, "Time");

  // Add Features to Model 1 / Firmware v1 using sMultiplicationConstants link
  int sfNumTcV1 = manager->addScreenFeature(smfV1MultConst, fNumTc, "NUM_TC");
  int sfDenTcV1 = manager->addScreenFeature(smfV1MultConst, fDenTc, "DEN_TC");
  int sfNumTpV1 = manager->addScreenFeature(smfV1MultConst, fNumTp, "NUM_TP");
  int sfDenTpV1 = manager->addScreenFeature(smfV1MultConst, fDenTp, "DEN_TP");

  int fcNmTcV1 = manager->linkScreenFeatureToComponent(sfNumTcV1, "Integer");
  int fcDnTcV1 = manager->linkScreenFeatureToComponent(sfDenTcV1, "Integer");
  int fcNmTpV1 = manager->linkScreenFeatureToComponent(sfNumTpV1, "Integer");
  int fcDnTpV1 = manager->linkScreenFeatureToComponent(sfDenTpV1, "Integer");

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
  int sfDateV2 = manager->addScreenFeature(smfV2DateTime, fDate, "DATE");
  int sfTimeV2 = manager->addScreenFeature(smfV2DateTime, fTime, "TIME");
  int sfGmtV2 = manager->addScreenFeature(smfV2DateTime, fGmt, "GMT");

  manager->linkScreenFeatureToComponent(sfDateV2, "Date");
  manager->linkScreenFeatureToComponent(sfTimeV2, "Time");
  int fcGmtV2 = manager->linkScreenFeatureToComponent(sfGmtV2, "Spinner");

  manager->addComponentLimit(fcGmtV2, "MIN_VALUE", "-12");
  manager->addComponentLimit(fcGmtV2, "MAX_VALUE", "12");
  manager->addComponentLimit(fcGmtV2, "DEFAULT_VALUE", "0");
  manager->addComponentLimit(fcGmtV2, "STEP", "1");

  // Add Layout features (0 feature_id)
  int sfTcWindowV2 = manager->addScreenFeature(smfV2MultConst, 0, "TC_WINDOW");
  int sfTpWindowV2 = manager->addScreenFeature(smfV2MultConst, 0, "TP_WINDOW");

  int sfNumTcV2 =
      manager->addScreenFeature(smfV2MultConst, fNumTc, "NUM_TC", sfTcWindowV2);
  int sfDenTcV2 =
      manager->addScreenFeature(smfV2MultConst, fDenTc, "DEN_TC", sfTcWindowV2);
  int sfNumTpV2 =
      manager->addScreenFeature(smfV2MultConst, fNumTp, "NUM_TP", sfTpWindowV2);
  int sfDenTpV2 =
      manager->addScreenFeature(smfV2MultConst, fDenTp, "DEN_TP", sfTpWindowV2);

  manager->linkScreenFeatureToComponent(sfTcWindowV2, "Window");
  manager->linkScreenFeatureToComponent(sfTpWindowV2, "Window");
  int fcNmTcV2 = manager->linkScreenFeatureToComponent(sfNumTcV2, "Integer");
  int fcDnTcV2 = manager->linkScreenFeatureToComponent(sfDenTcV2, "Integer");
  int fcNmTpV2 = manager->linkScreenFeatureToComponent(sfNumTpV2, "Integer");
  int fcDnTpV2 = manager->linkScreenFeatureToComponent(sfDenTpV2, "Integer");

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
  int dmfV3M1 = manager->linkFirmwareToModel(fwV3, m3p4w);
  int sAdvanced = manager->addService("ADVANCED_SETTINGS", 0);

  int smfV3Adv = manager->linkServiceToModelFirmware(sAdvanced, dmfV3M1);

  // Create a Window Layout element (0 feature_id)
  int sfWindowV3 = manager->addScreenFeature(smfV3Adv, 0, "SETTINGS_WINDOW");
  manager->linkScreenFeatureToComponent(sfWindowV3, "Window");

  // Add child features to the Window
  int sfWinDateV3 =
      manager->addScreenFeature(smfV3Adv, fDate, "WIN_DATE", sfWindowV3);
  int sfWinTimeV3 =
      manager->addScreenFeature(smfV3Adv, fTime, "WIN_TIME", sfWindowV3);

  manager->linkScreenFeatureToComponent(sfWinDateV3, "Date");
  manager->linkScreenFeatureToComponent(sfWinTimeV3, "Time");

  // Create Zeus V1 Physical Device
  Device zeusV1_a;
  zeusV1_a.name = "Zeus V1 Unit A";
  zeusV1_a.model_firmware_id = dmfV1M1;
  zeusV1_a.identifier = "SN-ZV1-001";
  zeusV1_a.description = "Zeus Unit running Firmware V1";
  zeusV1_a.is_template = false;
  int64_t zid1 = manager->addDevice(zeusV1_a);
  if (zid1 > 0) {
    // Record 1: Original deployment
    int64_t zr1_1 = manager->addDeviceRecord(zid1);
    manager->setRecordValue(zr1_1, fDate, "2023-01-01");
    manager->setRecordValue(zr1_1, fTime, "12:00:00");

    // Record 2: Minor time sync update
    int64_t zr1_2 = manager->addDeviceRecord(zid1);
    manager->setRecordValue(zr1_2, fDate, "2023-01-01");
    manager->setRecordValue(zr1_2, fTime, "14:30:00");
  }

  // Create Zeus V2 Physical Device
  Device zeusV2_b;
  zeusV2_b.name = "Zeus V2 Unit B";
  zeusV2_b.model_firmware_id = dmfV2M1;
  zeusV2_b.identifier = "SN-ZV2-002";
  zeusV2_b.description = "Zeus Unit running Firmware V2";
  zeusV2_b.is_template = false;
  int64_t zid2 = manager->addDevice(zeusV2_b);
  if (zid2 > 0) {
    // Record 1: Field configuration
    int64_t zr2_1 = manager->addDeviceRecord(zid2);
    manager->setRecordValue(zr2_1, fDate, "2024-02-11");
    manager->setRecordValue(zr2_1, fTime, "09:55:00");
    manager->setRecordValue(zr2_1, fGmt, "-3");

    // Record 2: Next day check-up
    int64_t zr2_2 = manager->addDeviceRecord(zid2);
    manager->setRecordValue(zr2_2, fDate, "2024-02-12");
    manager->setRecordValue(zr2_2, fTime, "10:00:00");
    manager->setRecordValue(zr2_2, fGmt, "-3");
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

  manager->addKeyWithTranslations("ENABLED",
                                  {{"enUs", "Enabled"}, {"ptBr", "Ativado"}});
  manager->addKeyWithTranslations(
      "DISABLED", {{"enUs", "Disabled"}, {"ptBr", "Desabilitado"}});

  manager->addKeyWithTranslations("RECLOSER_3P_v1.0.0",
                                  {{"enUs", "v1.0.0"}, {"ptBr", "v1.0.0"}});
  // Add Lines
  int dRecloser3p = manager->addLine("RECLOSER_3P"); // ID 1

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

  // Add Features (Generic Parameters)
  int fEnabledAutomaticReset = manager->addFeature("ENABLED_AUTOMATIC_RESET");
  int fAutomaticResetTime = manager->addFeature("AUTOMATIC_RESET_TIME");
  int fEnabledOvervoltageStage1 =
      manager->addFeature("ENABLED_OVERVOLTAGE_STAGE_1");
  int fOvervoltageStage1Threshold =
      manager->addFeature("OVERVOLTAGE_STAGE_1_THRESHOLD");
  int fRecloseAttempts = manager->addFeature("AUTOMATIC_RECLOSE_ATTEMPTS");

  // Instantiate Screen Features for Model R3P / Firmware v1
  int sfBasProtReset = manager->addScreenFeature(
      smfBasProt, fEnabledAutomaticReset, "ENABLED_AUTOMATIC_RESET");
  int sfBasProtTime = manager->addScreenFeature(smfBasProt, fAutomaticResetTime,
                                                "AUTOMATIC_RESET_TIME");

  int fcBasProtReset =
      manager->linkScreenFeatureToComponent(sfBasProtReset, "Toggle");
  int fcBasProtTime =
      manager->linkScreenFeatureToComponent(sfBasProtTime, "Time");

  manager->addComponentOption(fcBasProtReset, "1", "ENABLED");
  manager->addComponentOption(fcBasProtReset, "0", "DISABLED");
  manager->addComponentLimit(fcBasProtTime, "DEFAULT_VALUE", "5");

  int sfOvVReset = manager->addScreenFeature(
      smfOvVstage1, fEnabledOvervoltageStage1, "ENABLED_OVERVOLTAGE_STAGE_1");
  int sfOvVThreshold =
      manager->addScreenFeature(smfOvVstage1, fOvervoltageStage1Threshold,
                                "OVERVOLTAGE_STAGE_1_THRESHOLD");

  int fcOvVReset = manager->linkScreenFeatureToComponent(sfOvVReset, "Toggle");
  int fcOvVThreshold =
      manager->linkScreenFeatureToComponent(sfOvVThreshold, "Time");

  manager->addComponentOption(fcOvVReset, "1", "ENABLED");
  manager->addComponentOption(fcOvVReset, "0", "DISABLED");
  manager->addComponentLimit(fcOvVThreshold, "DEFAULT_VALUE", "5");

  int sfRecloseAttempts = manager->addScreenFeature(
      smfBasProt, fRecloseAttempts, "AUTOMATIC_RECLOSE_ATTEMPTS");
  int fcRecloseAttempts =
      manager->linkScreenFeatureToComponent(sfRecloseAttempts, "ComboBox");

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
  Device dev1;
  dev1.name = "Recloser Unit A";
  dev1.model_firmware_id = dmfV1R3P;
  dev1.identifier = "SN-R3P-001";
  dev1.description = "Primary Recloser for Substation Alpha";
  dev1.comment = "Initial deployment";
  dev1.is_template = false;
  int64_t id1 = manager->addDevice(dev1);

  if (id1 > 0) {
    // Record 1: Initial state
    int64_t rec1 = manager->addDeviceRecord(id1);
    manager->setRecordValue(rec1, fEnabledAutomaticReset, "1");
    manager->setRecordValue(rec1, fAutomaticResetTime, "10");
    manager->setRecordValue(rec1, fRecloseAttempts, "3");

    // Record 2: Updated state (Simulate a change later)
    int64_t rec2 = manager->addDeviceRecord(id1);
    manager->setRecordValue(rec2, fEnabledAutomaticReset, "1");
    manager->setRecordValue(rec2, fAutomaticResetTime, "15"); // Changed
    manager->setRecordValue(rec2, fRecloseAttempts, "2");     // Changed
  }
}
