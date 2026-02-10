'use server';

import { client, promisifyGrpc } from '@/lib/grpc';

export async function getInventory() {
    try {
        const response = await promisifyGrpc<any>(client.GetFullInventory, {});
        return response.devices || [];
    } catch (error) {
        console.error('Failed to fetch inventory:', error);
        return [];
    }
}

export async function getServiceTree(deviceId: number, modelId: number, firmwareId: number) {
    try {
        const response = await promisifyGrpc<any>(client.GetServiceTree, {
            device_id: deviceId,
            model_id: modelId,
            firmware_id: firmwareId
        });
        return response.top_level_services || [];
    } catch (error) {
        console.error('Failed to fetch service tree:', error);
        return [];
    }
}

export async function getScreenLayout(serviceDeviceModelFirmwareId: number) {
    try {
        const response = await promisifyGrpc<any>(client.GetScreenLayout, { service_device_model_firmware_id: serviceDeviceModelFirmwareId });
        return response.service_layout || null;
    } catch (error) {
        console.error('Failed to fetch screen layout:', error);
        return null;
    }
}

export async function getLanguages() {
    try {
        const response = await promisifyGrpc<any>(client.GetLanguages, {});
        return response.languages || [];
    } catch (error) {
        console.error('Failed to fetch languages:', error);
        return [];
    }
}
