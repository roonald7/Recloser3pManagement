'use client';

import { useState, useEffect } from 'react';
import { getScreenLayout } from './actions';
import {
    ChevronRight,
    Settings2,
    ToggleLeft,
    Monitor,
    Activity,
    ShieldCheck,
    LayoutGrid,
    Plus,
    Minus
} from 'lucide-react';

interface ConfiguratorProps {
    firmwareId: number;
    services: any[];
}

export default function Configurator({ firmwareId, services }: ConfiguratorProps) {
    const [selectedServiceId, setSelectedServiceId] = useState<number | null>(
        services.length > 0 ? services[0].id : null
    );
    const [layout, setLayout] = useState<any>(null);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        if (selectedServiceId) {
            loadLayout(selectedServiceId);
        }
    }, [selectedServiceId]);

    async function loadLayout(id: number) {
        setLoading(true);
        const result = await getScreenLayout(id);
        setLayout(result);
        setLoading(false);
    }

    const getTranslation = (translations: any[], lang: string = 'enUs') => {
        const t = translations?.find((t: any) => t.language_code === lang) || translations?.[0];
        return t?.value || 'N/A';
    };

    const getServiceIcon = (key: string) => {
        if (key.includes('PROT')) return <ShieldCheck size={18} />;
        if (key.includes('MEAS')) return <Activity size={18} />;
        return <Settings2 size={18} />;
    };

    const renderComponent = (feature: any) => {
        const type = feature.component_type?.toUpperCase();

        // Helper to get limit value by key
        const getLimit = (key: string) => feature.limits?.find((l: any) => l.key === key)?.value;

        const minValue = getLimit('MIN_VALUE');
        const maxValue = getLimit('MAX_VALUE');
        const defaultValue = getLimit('DEFAULT_VALUE');
        const stepValue = getLimit('STEP');
        const maxChar = getLimit('MAX_CHAR');
        const minChar = getLimit('MIN_CHAR');

        switch (type) {
            case 'COMBOBOX':
                return (
                    <select className="select-control" defaultValue={defaultValue}>
                        <option value="">Select Option...</option>
                        {/* Options would normally come from another table or metadata */}
                    </select>
                );
            case 'INTEGER':
            case 'DECIMAL':
            case 'FLOAT':
                return (
                    <input
                        type="number"
                        className="input-control"
                        placeholder={defaultValue || "0"}
                        min={minValue}
                        max={maxValue}
                        step={stepValue || (type === 'INTEGER' ? "1" : "0.01")}
                        defaultValue={defaultValue}
                        style={{ maxWidth: '200px' }}
                    />
                );
            case 'SPINNER':
                return (
                    <div className="numeric-spinner-container">
                        <button
                            className="spinner-btn"
                            onClick={(e) => {
                                const input = e.currentTarget.parentElement?.querySelector('input');
                                if (input) {
                                    input.stepDown();
                                    input.dispatchEvent(new Event('change', { bubbles: true }));
                                }
                            }}
                        >
                            <Minus size={14} />
                        </button>
                        <input
                            type="number"
                            className="input-control spinner-input"
                            placeholder={defaultValue || "0"}
                            min={minValue}
                            max={maxValue}
                            step={stepValue || "1"}
                            defaultValue={defaultValue}
                        />
                        <button
                            className="spinner-btn"
                            onClick={(e) => {
                                const input = e.currentTarget.parentElement?.querySelector('input');
                                if (input) {
                                    input.stepUp();
                                    input.dispatchEvent(new Event('change', { bubbles: true }));
                                }
                            }}
                        >
                            <Plus size={14} />
                        </button>
                    </div>
                );
            case 'TEXTFIELD':
                return (
                    <input
                        type="text"
                        className="input-control"
                        placeholder="Enter text..."
                        maxLength={maxChar ? parseInt(maxChar) : undefined}
                        minLength={minChar ? parseInt(minChar) : undefined}
                        defaultValue={defaultValue}
                    />
                );
            case 'TOGGLE':
                return <ToggleLeft size={32} style={{ cursor: 'pointer', color: 'var(--primary)' }} />;
            case 'CHECKBOX':
                return <input type="checkbox" style={{ width: '20px', height: '20px' }} defaultChecked={defaultValue === 'true'} />;
            case 'DATE':
                return <input type="date" className="input-control" defaultValue={defaultValue} />;
            case 'TIME':
                return <input type="time" className="input-control" defaultValue={defaultValue} />;
            case 'DATETIME':
                return <input type="datetime-local" className="input-control" defaultValue={defaultValue} />;
            default:
                return <input type="text" className="input-control" placeholder="Enter value..." defaultValue={defaultValue} />;
        }
    };

    const renderFeatureBlock = (feat: any) => {
        const isContainer = feat.children && feat.children.length > 0;

        return (
            <div key={feat.feature_id} className={isContainer ? "container-block" : "feature-form-item"} style={isContainer ? { gridColumn: '1 / -1', marginTop: '1rem' } : {}}>
                {isContainer ? (
                    <div className="container-wrapper" style={{
                        border: '1px solid var(--glass-border)',
                        borderRadius: '12px',
                        padding: '1.5rem',
                        background: 'rgba(255,255,255,0.03)'
                    }}>
                        <div className="label-container" style={{ marginBottom: '1rem', borderBottom: '1px solid var(--glass-border)', paddingBottom: '0.5rem' }}>
                            <div className="feature-label" style={{ fontSize: '1.1rem', color: 'var(--accent)' }}>
                                {getTranslation(feat.translations)}
                            </div>
                            <div className="badge" style={{ fontSize: '0.65rem' }}>
                                {feat.component_type}
                            </div>
                        </div>
                        <div className="feature-grid">
                            {feat.children.map((child: any) => renderFeatureBlock(child))}
                        </div>
                    </div>
                ) : (
                    <>
                        <div className="label-container">
                            <div className="feature-label">
                                {getTranslation(feat.translations)}
                            </div>
                            <div className="badge" style={{ color: 'var(--text-muted)', fontSize: '0.65rem' }}>
                                {feat.component_type}
                            </div>
                        </div>
                        <div className="control-container">
                            {renderComponent(feat)}
                        </div>
                    </>
                )}
            </div>
        );
    };

    const renderLayoutContent = (data: any, depth: number = 0) => {
        if (!data) return null;

        const hasFeatures = data.features && data.features.length > 0;
        const hasChildren = data.children && data.children.length > 0;

        return (
            <div key={data.service_id} style={{ marginBottom: depth > 0 ? '2rem' : '0' }}>
                {depth > 0 && (
                    <div style={{
                        display: 'flex',
                        alignItems: 'center',
                        gap: '0.5rem',
                        marginBottom: '1.25rem',
                        paddingBottom: '0.5rem',
                        borderBottom: '1px solid var(--glass-border)'
                    }}>
                        <LayoutGrid size={16} color="var(--accent)" />
                        <h3 style={{ fontSize: '1rem', fontWeight: 600, color: 'var(--accent)' }}>
                            {getTranslation(data.translations)}
                        </h3>
                    </div>
                )}

                {hasFeatures && (
                    <div className="feature-grid">
                        {data.features.map((feat: any) => renderFeatureBlock(feat))}
                    </div>
                )}

                {hasChildren && (
                    <div style={{ marginLeft: depth > 0 ? '1rem' : '0' }}>
                        {data.children.map((child: any) => renderLayoutContent(child, depth + 1))}
                    </div>
                )}

                {!hasFeatures && !hasChildren && depth === 0 && (
                    <div style={{ textAlign: 'center', padding: '4rem', opacity: 0.5 }}>
                        <Monitor size={48} style={{ marginBottom: '1rem' }} />
                        <p>No interactive features or sub-sections defined.</p>
                    </div>
                )}
            </div>
        );
    };

    return (
        <div className="configurator-layout">
            {/* Side Menu */}
            <aside className="sidebar">
                <h2 className="section-title" style={{ padding: '0 1rem', marginBottom: '1.5rem' }}>Services</h2>
                {services.map((svc) => (
                    <div
                        key={svc.id}
                        className={`sidebar-item ${selectedServiceId === svc.id ? 'active' : ''}`}
                        onClick={() => setSelectedServiceId(svc.id)}
                    >
                        {getServiceIcon(svc.description_key)}
                        <div style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
                            <span style={{ fontWeight: 500 }}>{getTranslation(svc.translations)}</span>
                        </div>
                        <ChevronRight size={14} opacity={0.5} />
                    </div>
                ))}
            </aside>

            {/* Main Content Area */}
            <main className="content-area">
                {loading ? (
                    <div className="loading">Loading Configuration...</div>
                ) : layout ? (
                    <div>
                        <div style={{ marginBottom: '2.5rem' }}>
                            <h2 style={{ fontSize: '1.75rem', fontWeight: 700 }}>
                                {getTranslation(layout.translations)}
                            </h2>
                        </div>

                        {renderLayoutContent(layout)}
                    </div>
                ) : (
                    <div className="loading">Select a service to begin configuration</div>
                )}
            </main>
        </div>
    );
}
