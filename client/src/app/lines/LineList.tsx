'use client';

import React, { useState } from 'react';
import { HardDrive, Scale, Info, Check, Plus, Trash2, History, Calendar, Clock, ChevronLeft, ChevronRight } from 'lucide-react';
import Link from 'next/link';
import DeviceComparison from '../../components/DeviceComparison';
import { listDeviceRecords } from '../actions';

interface DeviceListProps {
    initialDevices: any[];
    inventory: any[];
    currentLang: string;
}

export default function DeviceList({ initialDevices, inventory, currentLang }: DeviceListProps) {
    const [selectedForComparison, setSelectedForComparison] = useState<number[]>([]);
    const [showComparison, setShowComparison] = useState(false);

    // History states
    const [historyDeviceId, setHistoryDeviceId] = useState<number | null>(null);
    const [historyRecords, setHistoryRecords] = useState<any[]>([]);
    const [selectedRecords, setSelectedRecords] = useState<number[]>([]);
    const [showRecordComparison, setShowRecordComparison] = useState(false);
    const [loadingHistory, setLoadingHistory] = useState(false);

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

    const getTranslation = (data: any, lang: string = currentLang) => {
        const translations = data?.translations || data?.info?.translations;
        const t = translations?.find((t: any) => t.language_code === lang) || translations?.[0];
        return t?.value || data?.description_key || data?.info?.description_key || 'N/A';
    };

    const getInventoryName = (lineId: number) => {
        const devMeta = inventory.find(d => d.id === lineId);
        if (!devMeta) return 'Unknown Line';
        return getTranslation(devMeta);
    };

    const findIdsByMfId = (mfId: number) => {
        for (const line of inventory) {
            for (const model of line.models) {
                for (const fw of model.firmwares) {
                    if (fw.id === mfId) {
                        return { lineId: line.id, modelId: model.id, firmwareId: fw.id };
                    }
                }
            }
        }
        return null;
    };

    const handleViewHistory = async (deviceId: number) => {
        setHistoryDeviceId(deviceId);
        setLoadingHistory(true);
        setSelectedRecords([]);
        setShowRecordComparison(false);
        const records = await listDeviceRecords(deviceId);
        setHistoryRecords(records);
        setLoadingHistory(false);
    };

    const toggleRecordSelection = (rid: number) => {
        if (selectedRecords.includes(rid)) {
            setSelectedRecords(selectedRecords.filter(i => i !== rid));
        } else {
            if (selectedRecords.length < 2) {
                setSelectedRecords([...selectedRecords, rid]);
            } else {
                setSelectedRecords([selectedRecords[1], rid]);
            }
        }
    };

    const getRecordName = (rid: number) => {
        const rec = historyRecords.find(r => r.id === rid);
        if (!rec) return `Record #${rid}`;
        return new Date(rec.created_at).toLocaleString();
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

    if (showRecordComparison && selectedRecords.length === 2) {
        return (
            <DeviceComparison
                id1={selectedRecords[0]}
                id2={selectedRecords[1]}
                deviceName1={getRecordName(selectedRecords[0])}
                deviceName2={getRecordName(selectedRecords[1])}
                languageCode={currentLang}
                type="record"
                onClose={() => setShowRecordComparison(false)}
            />
        );
    }

    if (historyDeviceId !== null) {
        const device = initialDevices.find(d => d.id === historyDeviceId);
        return (
            <div>
                <div style={{ display: 'flex', alignItems: 'center', gap: '1rem', marginBottom: '2rem' }}>
                    <button onClick={() => setHistoryDeviceId(null)} className="back-button" style={{ marginBottom: 0 }}>
                        <ChevronLeft size={16} />
                    </button>
                    <div>
                        <h2 style={{ fontSize: '1.25rem', marginBottom: '0.25rem' }}>Configuration History: {device?.name}</h2>
                        <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>Select two records to compare changes</p>
                    </div>
                </div>

                {loadingHistory ? (
                    <div className="card" style={{ padding: '4rem', textAlign: 'center' }}>
                        <div className="spinner-btn" style={{ margin: '0 auto', width: '2rem', height: '2rem' }}></div>
                        <p style={{ marginTop: '1rem', color: 'var(--text-muted)' }}>Loading records...</p>
                    </div>
                ) : (
                    <div style={{ display: 'flex', flexDirection: 'column', gap: '1.5rem' }}>
                        {selectedRecords.length > 0 && (
                            <div className="card" style={{ padding: '1.5rem', background: 'var(--primary-glow)', borderColor: 'var(--primary)', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                                <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
                                    <div style={{ padding: '0.75rem', borderRadius: '12px', background: 'var(--card-bg)' }}>
                                        <Scale size={20} color="var(--primary)" />
                                    </div>
                                    <div>
                                        <div style={{ fontSize: '0.875rem', color: 'var(--text-muted)' }}>Comparison Mode</div>
                                        <div style={{ fontWeight: 600 }}>{selectedRecords.length} / 2 records selected</div>
                                    </div>
                                </div>
                                <div style={{ display: 'flex', gap: '0.75rem' }}>
                                    <button
                                        onClick={() => setSelectedRecords([])}
                                        style={{ background: 'transparent', border: '1px solid var(--glass-border)', color: 'var(--foreground)', padding: '0.5rem 1rem', borderRadius: '8px', cursor: 'pointer' }}
                                    >
                                        Clear Selection
                                    </button>
                                    <button
                                        disabled={selectedRecords.length !== 2}
                                        onClick={() => setShowRecordComparison(true)}
                                        style={{
                                            background: selectedRecords.length === 2 ? 'var(--primary)' : 'var(--glass)',
                                            color: selectedRecords.length === 2 ? 'white' : 'var(--text-muted)',
                                            border: 'none',
                                            padding: '0.5rem 1.5rem',
                                            borderRadius: '8px',
                                            cursor: selectedRecords.length === 2 ? 'pointer' : 'not-allowed',
                                            fontWeight: 600
                                        }}
                                    >
                                        Compare Records
                                    </button>
                                </div>
                            </div>
                        )}

                        <div className="inventory-grid">
                            {historyRecords.length === 0 ? (
                                <div className="card" style={{ gridColumn: '1/-1', padding: '4rem', textAlign: 'center' }}>
                                    <History size={48} color="var(--text-muted)" style={{ margin: '0 auto 1rem', opacity: 0.5 }} />
                                    <p style={{ color: 'var(--text-muted)' }}>No historical records found for this device.</p>
                                </div>
                            ) : (
                                historyRecords.map((record: any) => {
                                    const isSelected = selectedRecords.includes(record.id);
                                    const date = new Date(record.created_at);
                                    return (
                                        <div
                                            key={record.id}
                                            className={`card ${isSelected ? 'active-card' : ''}`}
                                            onClick={() => toggleRecordSelection(record.id)}
                                            style={{ cursor: 'pointer', borderColor: isSelected ? 'var(--primary)' : 'var(--glass-border)' }}
                                        >
                                            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: '1.5rem' }}>
                                                <div style={{ display: 'flex', gap: '1rem' }}>
                                                    <div style={{ width: '40px', height: '40px', borderRadius: '10px', background: 'var(--glass)', display: 'flex', alignItems: 'center', justifyContent: 'center', color: 'var(--accent)' }}>
                                                        <Calendar size={20} />
                                                    </div>
                                                    <div>
                                                        <div style={{ fontWeight: 600 }}>Snapshot</div>
                                                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>ID: {record.id}</div>
                                                    </div>
                                                </div>
                                                <div style={{
                                                    width: '24px',
                                                    height: '24px',
                                                    borderRadius: '50%',
                                                    border: '2px solid',
                                                    borderColor: isSelected ? 'var(--primary)' : 'var(--glass-border)',
                                                    background: isSelected ? 'var(--primary)' : 'transparent',
                                                    display: 'flex',
                                                    alignItems: 'center',
                                                    justifyContent: 'center'
                                                }}>
                                                    {isSelected && <Check size={14} color="white" />}
                                                </div>
                                            </div>

                                            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                                                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', fontSize: '0.9rem' }}>
                                                    <Calendar size={14} color="var(--text-muted)" />
                                                    <span>{date.toLocaleDateString()}</span>
                                                </div>
                                                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', fontSize: '0.9rem' }}>
                                                    <Clock size={14} color="var(--text-muted)" />
                                                    <span>{date.toLocaleTimeString()}</span>
                                                </div>
                                            </div>

                                            <div style={{ marginTop: '1.5rem', paddingTop: '1rem', borderTop: '1px solid var(--glass-border)', fontSize: '0.8rem', color: 'var(--text-muted)', display: 'flex', justifyContent: 'space-between' }}>
                                                <span>{record.values?.length || 0} parameters</span>
                                                <span style={{ color: 'var(--accent)' }}>Select <ChevronRight size={12} /></span>
                                            </div>
                                        </div>
                                    );
                                })
                            )}
                        </div>
                    </div>
                )}
            </div>
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
                                    <span style={{ fontWeight: 500 }}>{getInventoryName(findIdsByMfId(device.model_firmware_id)?.lineId || 0)}</span>
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

                            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                                <div style={{ display: 'flex', gap: '0.5rem' }}>
                                    <Link
                                        href={{
                                            pathname: '/',
                                            query: {
                                                ...findIdsByMfId(device.model_firmware_id),
                                                deviceId: device.id,
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
                                        style={{ flex: 1, cursor: 'pointer', background: 'var(--glass-hover)', color: 'var(--foreground)', border: 'none', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: '0.5rem' }}
                                        onClick={() => handleViewHistory(device.id)}
                                    >
                                        <History size={14} /> History
                                    </button>
                                </div>
                                <button
                                    className="badge"
                                    style={{ width: '100%', cursor: 'pointer', background: isSelected ? 'var(--primary)' : 'var(--glass)', color: isSelected ? 'white' : 'var(--foreground)', border: 'none', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: '0.5rem' }}
                                    onClick={() => toggleSelection(device.id)}
                                >
                                    {isSelected ? <><Check size={14} /> Selected for Compare</> : <><Scale size={14} /> Compare</>}
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
