'use server';

import { client, recordClient, promisifyGrpc } from '@/lib/grpc';

export async function getInventory() {
    try {
        const response = await promisifyGrpc<any>(client, client.GetFullInventory, {});
        return response.lines || [];
    } catch (error) {
        console.error('Failed to fetch inventory:', error);
        return [];
    }
}

export async function getServiceTree(lineId: number, modelId: number, firmwareId: number) {
    try {
        const response = await promisifyGrpc<any>(client, client.GetServiceTree, {
            line_id: lineId,
            model_id: modelId,
            firmware_id: firmwareId
        });
        return response.top_level_services || [];
    } catch (error) {
        console.error('Failed to fetch service tree:', error);
        return [];
    }
}

export async function getScreenLayout(lineId: number, modelId: number, firmwareId: number, serviceId?: number, deviceId?: number) {
    try {
        const response = await promisifyGrpc<any>(client, client.GetScreenLayout, {
            line_id: lineId,
            model_id: modelId,
            firmware_id: firmwareId,
            service_id: serviceId,
            device_id: deviceId
        });
        return response.service_layout || null;
    } catch (error) {
        console.error('Failed to fetch screen layout:', error);
        return null;
    }
}

export async function getLanguages() {
    try {
        const response = await promisifyGrpc<any>(client, client.GetLanguages, {});
        return response.languages || [];
    } catch (error) {
        console.error('Failed to fetch languages:', error);
        return [];
    }
}

export async function getDevices() {
    try {
        const response = await promisifyGrpc<any>(client, client.GetAllDevices, {});
        return response.devices || [];
    } catch (error) {
        console.error('Failed to fetch devices:', error);
        return [];
    }
}

export async function compareDevices(id1: string | number, id2: string | number) {
    try {
        const response = await promisifyGrpc<any>(client, client.CompareDevices, {
            device_id_1: id1,
            device_id_2: id2
        });
        return response;
    } catch (error) {
        console.error('Failed to compare devices:', error);
        return null;
    }
}

export async function listDeviceRecords(deviceId: number) {
    try {
        const response = await promisifyGrpc<any>(recordClient, recordClient.ListDeviceRecords, {
            device_id: deviceId
        });
        return response.records || [];
    } catch (error) {
        console.error('Failed to list device records:', error);
        return [];
    }
}

export async function compareRecords(rid1: string | number, rid2: string | number) {
    try {
        const response = await promisifyGrpc<any>(recordClient, recordClient.CompareRecords, {
            record_id_1: rid1,
            record_id_2: rid2
        });
        return response;
    } catch (error) {
        console.error('Failed to compare records:', error);
        return null;
    }
}
