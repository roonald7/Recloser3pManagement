#include <iostream>
#include <vector>
#include <filesystem>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "RecloserManager.hpp"
#include "RecloserServiceImpl.hpp"

void RunServer(RecloserManager* manager, const std::string& server_address) {
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

    // Setup Languages
    manager.addLanguage("enUs", "English");
    manager.addLanguage("ptBr", "Português");
    manager.addLanguage("esEs", "Español");

    // Setup Description Keys
    manager.addDescriptionKey("VOLTAGE");
    manager.addDescriptionKey("CURRENT");
    manager.addDescriptionKey("FREQUENCY");
    manager.addDescriptionKey("DEVICE_NAME");
    manager.addDescriptionKey("STATUS_OPEN");
    manager.addDescriptionKey("STATUS_CLOSED");

    // English Translations
    manager.addTranslation("VOLTAGE", "enUs", "Voltage");
    manager.addTranslation("CURRENT", "enUs", "Current");
    manager.addTranslation("FREQUENCY", "enUs", "Frequency");
    manager.addTranslation("DEVICE_NAME", "enUs", "Device Name");
    manager.addTranslation("STATUS_OPEN", "enUs", "Open");
    manager.addTranslation("STATUS_CLOSED", "enUs", "Closed");

    // Portuguese Translations
    manager.addTranslation("VOLTAGE", "ptBr", "Tensão");
    manager.addTranslation("CURRENT", "ptBr", "Corrente");
    manager.addTranslation("FREQUENCY", "ptBr", "Frequência");
    manager.addTranslation("DEVICE_NAME", "ptBr", "Nome do Dispositivo");
    manager.addTranslation("STATUS_OPEN", "ptBr", "Aberto");
    manager.addTranslation("STATUS_CLOSED", "ptBr", "Fechado");

    // Setup Description Keys for Reclosers
    manager.addDescriptionKey("RECLOSER_MODEL_1");
    manager.addDescriptionKey("RECLOSER_MODEL_2");

    // Translations for Recloser Keys
    manager.addTranslation("RECLOSER_MODEL_1", "enUs", "Primary Distribution Recloser");
    manager.addTranslation("RECLOSER_MODEL_1", "ptBr", "Religador de Distribuição Primária");
    manager.addTranslation("RECLOSER_MODEL_2", "enUs", "Smart Grid Recloser");
    manager.addTranslation("RECLOSER_MODEL_2", "ptBr", "Religador de Rede Inteligente");

    // Add Reclosers
    manager.addRecloser("RECLOSER_MODEL_1", "Model 1");
    manager.addRecloser("RECLOSER_MODEL_2", "Model 2");

    // Add Firmware Versions
    // For Model 1 (ID 1)
    manager.addFirmwareVersion("v1.0.0", 1);
    manager.addFirmwareVersion("v2.1.2", 1);
    // For Model 2 (ID 2)
    manager.addFirmwareVersion("v1.1.0", 2);
    manager.addFirmwareVersion("v2.0.5", 2);

    std::cout << "\n--- Translations Preview ---" << std::endl;
    printf("%-20s | %-30s | %-30s\n", "Key", "English", "Portuguese");
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
    
    std::vector<std::string> keys = {"VOLTAGE", "CURRENT", "RECLOSER_MODEL_1", "RECLOSER_MODEL_2"};
    for (const auto& key : keys) {
        printf("%-20s | %-30s | %-30s\n", 
               key.c_str(), 
               manager.getTranslation(key, "enUs").c_str(), 
               manager.getTranslation(key, "ptBr").c_str());
    }

    // Setup Services and Sections (Hierarchy)
    manager.addDescriptionKey("SERV_PROTECTION");
    manager.addDescriptionKey("SERV_MEASUREMENT");
    manager.addDescriptionKey("SERV_PROT_PARAMS");
    manager.addDescriptionKey("SERV_MEAS_LOGS");

    manager.addTranslation("SERV_PROTECTION", "enUs", "Protection Services");
    manager.addTranslation("SERV_PROTECTION", "ptBr", "Serviços de Proteção");
    manager.addTranslation("SERV_MEASUREMENT", "enUs", "Measurement Services");
    manager.addTranslation("SERV_MEASUREMENT", "ptBr", "Serviços de Medição");

    manager.addTranslation("SERV_PROT_PARAMS", "enUs", "Protection Parameters");
    manager.addTranslation("SERV_PROT_PARAMS", "ptBr", "Parâmetros de Proteção");
    manager.addTranslation("SERV_MEAS_LOGS", "enUs", "Measurement Logs");
    manager.addTranslation("SERV_MEAS_LOGS", "ptBr", "Logs de Medição");

    // 1. Setup Services for Firmware 1 (v1.0.0)
    manager.addService("SEC_PROT_V1", "SERV_PROTECTION", 1, 0);  // ID 1
    manager.addService("PROT_PARAMS_V1", "SERV_PROT_PARAMS", 1, 1); // ID 2
    
    // 2. Setup Services for Firmware 2 (v2.1.2) - Duplicating + adding
    manager.addService("SEC_PROT_V2", "SERV_PROTECTION", 2, 0);  // ID 3
    manager.addService("PROT_PARAMS_V2", "SERV_PROT_PARAMS", 2, 3); // ID 4

    // Setup Feature Description Keys
    manager.addDescriptionKey("FEAT_OVERCURRENT");
    manager.addDescriptionKey("FEAT_RECLOSE_LIMIT");
    manager.addDescriptionKey("FEAT_OSCILLOGRAPHY");

    manager.addTranslation("FEAT_OVERCURRENT", "enUs", "Overcurrent Protection");
    manager.addTranslation("FEAT_OVERCURRENT", "ptBr", "Proteção de Sobrecorrente");
    manager.addTranslation("FEAT_RECLOSE_LIMIT", "enUs", "Reclose Count Limit");
    manager.addTranslation("FEAT_RECLOSE_LIMIT", "ptBr", "Limite de Contagem de Religamento");
    manager.addTranslation("FEAT_OSCILLOGRAPHY", "enUs", "Advanced Oscillography");
    manager.addTranslation("FEAT_OSCILLOGRAPHY", "ptBr", "Oscilografia Avançada");

    // Add Features for V1 (Service ID 2)
    manager.addFeature("FEAT_OVERCURRENT", 2);
    manager.addFeature("FEAT_RECLOSE_LIMIT", 2);

    // Add Features for V2 (Service ID 4) - Same as V1 + Oscillography
    manager.addFeature("FEAT_OVERCURRENT", 4);
    manager.addFeature("FEAT_RECLOSE_LIMIT", 4);
    manager.addFeature("FEAT_OSCILLOGRAPHY", 4); // The additional feature for V2

    std::cout << "\n--- Full Recloser Hierarchy (Model -> Firmware -> Service Tree -> Features) ---" << std::endl;
    auto allReclosers = manager.getAllReclosers();
    for (const auto& r : allReclosers) {
        std::cout << "\n[Recloser] " << r.model << " (" << manager.getTranslation(r.description_key, "ptBr") << ")" << std::endl;
        
        auto firmwares = manager.getFirmwareVersionsForRecloser(r.id);
        for (const auto& f : firmwares) {
            std::cout << "  └─ [Firmware] " << f.version << std::endl;
            
            // Get top-level services for this firmware
            auto topServices = manager.getServicesByParentAndFirmware(0, f.id);
            for (const auto& parent : topServices) {
                std::cout << "    ├─ [Section] " << manager.getTranslation(parent.description_key, "ptBr") << std::endl;
                
                // Get children services
                auto children = manager.getServicesByParentAndFirmware(parent.id, f.id);
                for (const auto& child : children) {
                    std::cout << "    │  └─ [Service] " << manager.getTranslation(child.description_key, "ptBr") << std::endl;
                    
                    // Show features for this service
                    auto feats = manager.getFeaturesByService(child.id);
                    for (const auto& feat : feats) {
                        std::cout << "    │     * [Feature] " << manager.getTranslation(feat.description_key, "ptBr") << std::endl;
                    }
                }
            }
        }
    }

    // Start gRPC server in a separate thread
    std::cout << "\n--- Starting gRPC Server ---" << std::endl;
    std::string server_address("0.0.0.0:50051");
    
    std::thread server_thread(RunServer, &manager, server_address);
    
    std::cout << "\nPress Ctrl+C to stop the server..." << std::endl;
    
    server_thread.join();

    return 0;
}
