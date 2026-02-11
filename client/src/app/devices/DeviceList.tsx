'use client';

import React, { useState } from 'react';
import { HardDrive, Scale, Info, Check, Plus, Trash2 } from 'lucide-react';
import Link from 'next/link';
import DeviceComparison from '../../components/DeviceComparison';


interface DeviceListProps {
    initialDevices: any[];
    inventory: any[];
    currentLang: string;
}

export default function DeviceList({ initialDevices, inventory, currentLang }: DeviceListProps) {
    const [selectedForComparison, setSelectedForComparison] = useState<number[]>([]);
    const [showComparison, setShowComparison] = useState(false);

    const toggleSelection = (id: number) => {
        if (selectedForComparison.includes(id)) {
            setSelectedForComparison(selectedForComparison.filter(i => i !== id));
        } else {
            if (selectedForComparison.length < 2) {
                setSelectedForComparison([...selectedForComparison, id]);
            } else {
                setSelectedForComparison([selectedForComparison[1], id]);
            }
        }
    };

    const getDeviceName = (id: number) => {
        return initialDevices.find(d => d.id === id)?.name || `Device ${id}`;
    };

    const getInventoryName = (deviceId: number) => {
        const devMeta = inventory.find(d => d.id === deviceId);
        if (!devMeta) return 'Unknown Device';
        const t = devMeta.translations?.find((t: any) => t.language_code === currentLang) || devMeta.translations?.[0];
        return t?.value || 'N/A';
    };

    if (showComparison && selectedForComparison.length === 2) {
        return (
            <DeviceComparison
                id1={selectedForComparison[0]}
                id2={selectedForComparison[1]}
                deviceName1={getDeviceName(selectedForComparison[0])}
                deviceName2={getDeviceName(selectedForComparison[1])}
                languageCode={currentLang}
                onClose={() => setShowComparison(false)}
            />
        );
    }

    return (
        <div>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '2rem' }}>
                <h2 style={{ fontSize: '1.25rem' }}>Current Fleet ({initialDevices.length})</h2>

                <div style={{ display: 'flex', gap: '1rem' }}>
                    {selectedForComparison.length > 0 && (
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1rem', background: 'var(--glass)', padding: '0.5rem 1rem', borderRadius: '0.75rem', border: '1px solid var(--glass-border)' }}>
                            <span style={{ fontSize: '0.875rem', color: 'var(--text-muted)' }}>
                                {selectedForComparison.length} selected for comparison
                            </span>
                            <button
                                disabled={selectedForComparison.length !== 2}
                                onClick={() => setShowComparison(true)}
                                className="badge"
                                style={{
                                    background: selectedForComparison.length === 2 ? 'var(--primary)' : 'transparent',
                                    color: selectedForComparison.length === 2 ? 'white' : 'var(--text-muted)',
                                    cursor: selectedForComparison.length === 2 ? 'pointer' : 'not-allowed',
                                    opacity: selectedForComparison.length === 2 ? 1 : 0.5,
                                    display: 'flex',
                                    alignItems: 'center',
                                    gap: '0.5rem',
                                    border: 'none',
                                    padding: '0.5rem 1rem'
                                }}
                            >
                                <Scale size={16} /> Compare Selected
                            </button>
                            <button
                                onClick={() => setSelectedForComparison([])}
                                style={{ background: 'transparent', border: 'none', color: 'var(--text-muted)', cursor: 'pointer', fontSize: '0.75rem' }}
                            >
                                Clear
                            </button>
                        </div>
                    )}
                </div>
            </div>

            <div className="inventory-grid">
                {initialDevices.map((device: any) => {
                    const isSelected = selectedForComparison.includes(device.id);
                    return (
                        <div
                            key={device.id}
                            className={`card ${isSelected ? 'active-card' : ''}`}
                            style={{
                                borderColor: isSelected ? 'var(--primary)' : 'var(--glass-border)',
                                cursor: 'default'
                            }}
                        >
                            <div className="card-header">
                                <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
                                    <div style={{
                                        width: '40px',
                                        height: '40px',
                                        borderRadius: '10px',
                                        background: device.is_template ? 'var(--primary-glow)' : 'var(--glass)',
                                        display: 'flex',
                                        alignItems: 'center',
                                        justifyContent: 'center',
                                        color: device.is_template ? 'var(--primary)' : 'var(--text-muted)'
                                    }}>
                                        <HardDrive size={20} />
                                    </div>
                                    <div>
                                        <h3 style={{ margin: 0 }}>{device.name}</h3>
                                        <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>ID: {device.identifier}</span>
                                    </div>
                                </div>
                                <div
                                    onClick={() => toggleSelection(device.id)}
                                    style={{
                                        cursor: 'pointer',
                                        width: '24px',
                                        height: '24px',
                                        borderRadius: '50%',
                                        border: '2px solid',
                                        borderColor: isSelected ? 'var(--primary)' : 'var(--glass-border)',
                                        background: isSelected ? 'var(--primary)' : 'transparent',
                                        display: 'flex',
                                        alignItems: 'center',
                                        justifyContent: 'center'
                                    }}
                                >
                                    {isSelected && <Check size={14} color="white" />}
                                </div>
                            </div>

                            <div style={{ marginBottom: '1.5rem' }}>
                                <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.875rem', padding: '0.5rem 0', borderBottom: '1px solid var(--glass-border)' }}>
                                    <span style={{ color: 'var(--text-muted)' }}>Hardware Type</span>
                                    <span style={{ fontWeight: 500 }}>{getInventoryName(device.device_id)}</span>
                                </div>
                                {device.comment && (

                                    <p style={{ marginTop: '1rem', fontSize: '0.875rem', color: 'var(--text-muted)', fontStyle: 'italic' }}>
                                        "{device.comment}"
                                    </p>
                                )}
                            </div>

                            {device.is_template && (
                                <div className="badge" style={{ background: 'var(--primary-glow)', color: 'var(--primary)', border: 'none', marginBottom: '1rem' }}>
                                    Standard Template
                                </div>
                            )}

                            <div style={{ display: 'flex', gap: '0.5rem' }}>
                                <Link
                                    href={{
                                        pathname: '/',
                                        query: {
                                            deviceId: device.device_id,
                                            modelId: device.model_id,
                                            firmwareId: device.firmware_version_id,
                                            physicalDeviceId: device.id,
                                            lang: currentLang
                                        }
                                    }}
                                    className="badge"
                                    style={{ flex: 1, cursor: 'pointer', borderColor: 'var(--glass-border)', background: 'transparent', textDecoration: 'none', textAlign: 'center', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
                                >
                                    View Details
                                </Link>
                                <button
                                    className="badge"
                                    style={{ flex: 1, cursor: 'pointer', background: 'var(--glass-hover)', color: 'var(--foreground)', border: 'none' }}
                                    onClick={() => toggleSelection(device.id)}
                                >
                                    {isSelected ? 'Deselect' : 'Compare'}
                                </button>
                            </div>
                        </div>
                    );
                })}
            </div>

            <style jsx>{`
                .active-card {
                    box-shadow: 0 0 20px -5px var(--primary-glow);
                }
            `}</style>
        </div>
    );
}
