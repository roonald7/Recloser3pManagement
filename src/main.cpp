#include <iostream>
#include <vector>
#include <filesystem>
#include "RecloserManager.hpp"

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

    std::cout << "\n--- Reclosers & Firmwares List ---" << std::endl;
    
    auto reclosers = manager.getAllReclosers();
    for (const auto& r : reclosers) {
        std::cout << "--------------------------------------------------------------" << std::endl;
        printf("ID: %-2d | Model: %-10s | Description: %s\n", 
               r.id, 
               r.model.c_str(), 
               manager.getTranslation(r.description_key, "ptBr").c_str());
        
        auto firmwares = manager.getFirmwareVersionsForRecloser(r.id);
        std::cout << "  Firmware Versions:" << std::endl;
        for (const auto& f : firmwares) {
            printf("    - [%d] %s\n", f.id, f.version.c_str());
        }
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

    // 1. Add Parent Services (Sections)
    manager.addService("SEC_PROT", "SERV_PROTECTION", 0);     // Parent ID 1
    manager.addService("SEC_MEAS", "SERV_MEASUREMENT", 0);    // Parent ID 2

    // 2. Add Child Services
    manager.addService("PROT_PARAMS", "SERV_PROT_PARAMS", 1); // Child of Protection
    manager.addService("MEAS_LOGS", "SERV_MEAS_LOGS", 2);     // Child of Measurement

    std::cout << "\n--- Services Hierarchy ---" << std::endl;
    // Get top-level services
    auto topLevel = manager.getServicesByParent(0);
    for (const auto& parent : topLevel) {
        std::cout << "[Section] " << manager.getTranslation(parent.description_key, "ptBr") << std::endl;
        
        // Get children for this parent
        auto children = manager.getServicesByParent(parent.id);
        for (const auto& child : children) {
            std::cout << "  └─ [Service] " << manager.getTranslation(child.description_key, "ptBr") << std::endl;
        }
    }

    return 0;
}
