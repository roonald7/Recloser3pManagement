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

export async function getScreenLayout(deviceId: number, modelId: number, firmwareId: number, serviceId?: number, physicalDeviceId?: number) {
    try {
        const response = await promisifyGrpc<any>(client.GetScreenLayout, {
            device_id: deviceId,
            model_id: modelId,
            firmware_id: firmwareId,
            service_id: serviceId,
            physical_device_id: physicalDeviceId
        });
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

export async function getPhysicalDevices() {
    try {
        const response = await promisifyGrpc<any>(client.GetAllPhysicalDevices, {});
        return response.physical_devices || [];
    } catch (error) {
        console.error('Failed to fetch physical devices:', error);
        return [];
    }
}

export async function comparePhysicalDevices(id1: string | number, id2: string | number, languageCode: string = 'enUs') {
    try {
        const response = await promisifyGrpc<any>(client.ComparePhysicalDevices, {
            physical_device_id_1: id1,
            physical_device_id_2: id2,
            language_code: languageCode
        });
        return response;
    } catch (error) {
        console.error('Failed to compare physical devices:', error);
        return null;
    }
}

