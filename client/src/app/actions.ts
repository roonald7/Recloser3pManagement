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

export async function getServiceTree(firmwareId: number) {
    try {
        const response = await promisifyGrpc<any>(client.GetServiceTree, { firmware_id: firmwareId });
        return response.top_level_services || [];
    } catch (error) {
        console.error('Failed to fetch service tree:', error);
        return [];
    }
}

export async function getScreenLayout(serviceFirmwareId: number) {
    try {
        const response = await promisifyGrpc<any>(client.GetScreenLayout, { service_id: serviceFirmwareId });
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
