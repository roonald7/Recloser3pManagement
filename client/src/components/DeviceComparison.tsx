'use client';

import React, { useState, useEffect } from 'react';
import { compareDevices, compareRecords } from '../app/actions';
import { AlertCircle, CheckCircle2, ChevronRight, ChevronDown, Folder, Shield, Settings, Zap, History } from 'lucide-react';

interface DeviceComparisonProps {
    id1: number;
    id2: number;
    deviceName1: string;
    deviceName2: string;
    languageCode: string;
    type?: 'device' | 'record';
    onClose: () => void;
}

const getValueDisplay = (val: string, options: any[], lang: string) => {
    if (!val) return '—';
    const option = options?.find(o => String(o.value) === String(val));
    if (option) {
        const translation = option.translations?.find((t: any) =>
            (t.language_code?.toLowerCase() === lang.toLowerCase()) ||
            (t.languageCode?.toLowerCase() === lang.toLowerCase())
        );
        return translation?.value || option.value;
    }
    return val;
};

const getTranslation = (data: any, lang: string) => {
    const translations = data?.translations || data?.info?.translations;
    const t = translations?.find((t: any) => t.language_code === lang) || translations?.[0];
    return t?.value || data?.description_key || data?.info?.description_key || 'N/A';
};

const ServiceNode = ({ node, lang, level = 0 }: { node: any, lang: string, level?: number }) => {
    const [isOpen, setIsOpen] = useState(node.has_differences || level < 1);
    const title = getTranslation(node, lang);

    // Sort features: differences first
    const sortedFeatures = [...(node.features || [])].sort((a, b) => (b.is_different ? 1 : 0) - (a.is_different ? 1 : 0));

    return (
        <div style={{ marginLeft: level > 0 ? '1.5rem' : 0, marginBottom: '0.5rem' }}>
            <div
                className={`sidebar-item ${isOpen ? 'active' : ''}`}
                onClick={() => setIsOpen(!isOpen)}
                style={{
                    padding: '0.75rem',
                    display: 'flex',
                    alignItems: 'center',
                    gap: '0.75rem',
                    background: node.has_differences ? 'rgba(248, 113, 113, 0.05)' : 'var(--glass)',
                    borderColor: node.has_differences ? 'rgba(248, 113, 113, 0.2)' : 'var(--glass-border)'
                }}
            >
                {isOpen ? <ChevronDown size={16} /> : <ChevronRight size={16} />}
                <div style={{
                    width: '32px',
                    height: '32px',
                    borderRadius: '8px',
                    background: 'var(--glass)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    color: node.has_differences ? '#f87171' : 'var(--accent)'
                }}>
                    {level === 0 ? <Zap size={16} /> : <Folder size={16} />}
                </div>
                <span style={{ fontWeight: 600, flex: 1 }}>{title}</span>
                {node.has_differences && (
                    <span className="badge" style={{ background: '#f87171', color: 'white', border: 'none' }}>
                        Diff
                    </span>
                )}
            </div>

            {isOpen && (
                <div style={{ marginTop: '0.5rem' }}>
                    {/* Render Features */}
                    {sortedFeatures.map((feat: any) => (
                        <div key={feat.feature_id} style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', marginBottom: '0.5rem' }}>
                            <div
                                style={{
                                    padding: '1rem',
                                    background: 'var(--glass)',
                                    border: '1px solid',
                                    borderColor: feat.is_different ? 'rgba(248, 113, 113, 0.3)' : 'var(--glass-border)',
                                    borderRadius: '0.75rem',
                                    display: 'grid',
                                    gridTemplateColumns: '1fr 1fr 1fr',
                                    gap: '1rem',
                                    alignItems: 'center'
                                }}
                            >
                                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                                    {feat.is_different ? <AlertCircle size={14} color="#f87171" /> : <CheckCircle2 size={14} color="var(--accent)" />}
                                    <span style={{ fontSize: '0.9rem', fontWeight: 500 }}>
                                        {feat.info?.translations?.find((t: any) => t.language_code === lang)?.value || feat.info?.description_key}
                                    </span>
                                </div>
                                <div style={{ padding: '0.5rem', background: '#000', borderRadius: '0.5rem', textAlign: 'center', fontSize: '0.875rem' }}>
                                    {getValueDisplay(feat.value_1, feat.options, lang)}
                                </div>
                                <div style={{
                                    padding: '0.5rem',
                                    background: feat.is_different ? 'rgba(248, 113, 113, 0.1)' : '#000',
                                    borderRadius: '0.5rem',
                                    textAlign: 'center',
                                    fontSize: '0.875rem',
                                    color: feat.is_different ? '#f87171' : 'inherit',
                                    fontWeight: feat.is_different ? 600 : 400
                                }}>
                                    {getValueDisplay(feat.value_2, feat.options, lang)}
                                </div>
                            </div>
                            {feat.difference_note && (
                                <div style={{
                                    marginLeft: '3rem',
                                    fontSize: '0.75rem',
                                    color: '#f87171',
                                    fontWeight: 500,
                                    display: 'flex',
                                    alignItems: 'center',
                                    gap: '0.35rem'
                                }}>
                                    <AlertCircle size={12} />
                                    {feat.difference_note}
                                </div>
                            )}
                        </div>
                    ))}

                    {/* Render Children */}
                    {node.children?.map((child: any) => (
                        <ServiceNode key={child.service_id} node={child} lang={lang} level={level + 1} />
                    ))}
                </div>
            )}
        </div>
    );
};

export default function DeviceComparison({ id1, id2, deviceName1, deviceName2, languageCode, type = 'device', onClose }: DeviceComparisonProps) {
    const [comparison, setComparison] = useState<any>(null);
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        const fetchComparison = async () => {
            setLoading(true);
            let result;
            if (type === 'record') {
                result = await compareRecords(id1, id2);
            } else {
                result = await compareDevices(id1, id2);
            }
            setComparison(result);
            setLoading(false);
        };
        fetchComparison();
    }, [id1, id2, type]);

    if (loading) {
        return (
            <div className="card" style={{ padding: '4rem', textAlign: 'center' }}>
                <div className="spinner-btn" style={{ margin: '0 auto 1.5rem', width: '3rem', height: '3rem', cursor: 'default' }}>...</div>
                <p style={{ color: 'var(--text-muted)' }}>Building hierarchical comparison tree...</p>
            </div>
        );
    }

    const rootServices = comparison?.root_services || [];

    return (
        <div className="card" style={{ width: '100%', maxWidth: '1000px', margin: '0 auto' }}>
            <div className="card-header" style={{ marginBottom: '2rem' }}>
                <div>
                    <h2 style={{ fontSize: '1.5rem', marginBottom: '0.5rem' }}>
                        {type === 'record' ? 'Historical Record Comparison' : 'Service Tree Comparison'}
                    </h2>
                    <p style={{ color: 'var(--text-muted)' }}>
                        <span style={{ color: 'var(--accent)' }}>{deviceName1}</span> (Base) vs <span style={{ color: 'var(--primary)' }}>{deviceName2}</span>
                    </p>
                </div>
                <button onClick={onClose} className="back-button" style={{ marginBottom: 0 }}>
                    {type === 'record' ? 'Back to History' : 'Back to Fleet'}
                </button>
            </div>

            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: '1rem', padding: '1rem', marginBottom: '1.5rem', position: 'sticky', top: 0, zIndex: 10, background: 'var(--card-bg)', borderBottom: '1px solid var(--glass-border)' }}>
                <div style={{ color: 'var(--text-muted)', fontSize: '0.75rem', fontWeight: 600, textTransform: 'uppercase' }}>Structure / Parameter</div>
                <div style={{ color: 'var(--accent)', fontSize: '0.75rem', fontWeight: 600, textTransform: 'uppercase', textAlign: 'center' }}>{deviceName1}</div>
                <div style={{ color: 'var(--primary)', fontSize: '0.75rem', fontWeight: 600, textTransform: 'uppercase', textAlign: 'center' }}>{deviceName2}</div>
            </div>

            <div style={{ maxHeight: '70vh', overflowY: 'auto', paddingRight: '0.5rem' }} className="sidebar-scroll">
                {comparison?.root_services?.map((service: any) => (
                    <ServiceNode key={service.service_id} node={service} lang={languageCode} />
                ))}

                {comparison?.root_services?.length === 0 && (
                    <div style={{ padding: '4rem', textAlign: 'center' }}>
                        <p style={{ color: 'var(--text-muted)' }}>No service structure found for comparison.</p>
                    </div>
                )}
            </div>

            <div style={{ marginTop: '2rem', display: 'flex', gap: '1.5rem', fontSize: '0.875rem' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', color: 'var(--text-muted)' }}>
                    <div style={{ width: '12px', height: '12px', background: 'var(--accent)', borderRadius: '3px' }}></div>
                    Base Device
                </div>
                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem', color: 'var(--text-muted)' }}>
                    <div style={{ width: '12px', height: '12px', background: '#f87171', borderRadius: '3px' }}></div>
                    Difference Found
                </div>
            </div>
        </div>
    );
}
