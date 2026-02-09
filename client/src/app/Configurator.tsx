'use client';

import { useState, useEffect } from 'react';
import { getScreenLayout } from './actions';
import {
    ChevronRight,
    Settings2,
    ToggleLeft,
    ToggleRight,
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

    const ConfigField = ({ feature }: { feature: any }) => {
        const type = feature.component_type?.toUpperCase();
        const getLimit = (key: string) => feature.limits?.find((l: any) => l.key === key)?.value;
        const defaultValue = getLimit('DEFAULT_VALUE');

        const [value, setValue] = useState<any>(defaultValue);
        const [isToggled, setIsToggled] = useState(defaultValue === 'true' || defaultValue === '1' || defaultValue === 'ON');

        const renderInput = () => {
            const minValue = getLimit('MIN_VALUE');
            const maxValue = getLimit('MAX_VALUE');
            const stepValue = getLimit('STEP');
            const maxChar = getLimit('MAX_CHAR');
            const minChar = getLimit('MIN_CHAR');

            switch (type) {
                case 'COMBOBOX':
                    return (
                        <select
                            className="select-control"
                            value={value}
                            onChange={(e) => setValue(e.target.value)}
                        >
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
                            value={value || ''}
                            onChange={(e) => setValue(e.target.value)}
                            style={{ maxWidth: '200px' }}
                        />
                    );
                case 'SPINNER':
                    return (
                        <div className="numeric-spinner-container">
                            <button
                                className="spinner-btn"
                                onClick={() => {
                                    const num = parseFloat(value || defaultValue || "0");
                                    const step = parseFloat(stepValue || "1");
                                    setValue((num - step).toString());
                                }}
                            >
                                <Minus size={14} />
                            </button>
                            <input
                                type="number"
                                className="input-control spinner-input"
                                value={value || defaultValue || "0"}
                                readOnly
                            />
                            <button
                                className="spinner-btn"
                                onClick={() => {
                                    const num = parseFloat(value || defaultValue || "0");
                                    const step = parseFloat(stepValue || "1");
                                    setValue((num + step).toString());
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
                            value={value || ''}
                            onChange={(e) => setValue(e.target.value)}
                        />
                    );
                case 'TOGGLE':
                    return (
                        <div
                            onClick={() => setIsToggled(!isToggled)}
                            style={{ cursor: 'pointer', transition: 'all 0.2s' }}
                        >
                            {isToggled ? (
                                <ToggleRight size={32} color="var(--primary)" />
                            ) : (
                                <ToggleLeft size={32} color="var(--text-muted)" />
                            )}
                        </div>
                    );
                case 'CHECKBOX':
                    return (
                        <input
                            type="checkbox"
                            style={{ width: '20px', height: '20px', cursor: 'pointer' }}
                            checked={isToggled}
                            onChange={() => setIsToggled(!isToggled)}
                        />
                    );
                case 'DATE':
                    return <input type="date" className="input-control" value={value} onChange={(e) => setValue(e.target.value)} />;
                case 'TIME':
                    return <input type="time" className="input-control" value={value} onChange={(e) => setValue(e.target.value)} />;
                case 'DATETIME':
                    return <input type="datetime-local" className="input-control" value={value} onChange={(e) => setValue(e.target.value)} />;
                default:
                    return <input type="text" className="input-control" placeholder="Enter value..." value={value} onChange={(e) => setValue(e.target.value)} />;
            }
        };

        const isContainer = feature.children && feature.children.length > 0;

        return (
            <div key={feature.feature_id} className={isContainer ? "container-block" : "feature-form-item"} style={isContainer ? { gridColumn: '1 / -1', marginTop: '1rem' } : {}}>
                {isContainer ? (
                    <div className="container-wrapper" style={{
                        border: '1px solid var(--glass-border)',
                        borderRadius: '12px',
                        padding: '1.5rem',
                        background: 'rgba(255,255,255,0.03)'
                    }}>
                        <div className="label-container" style={{ marginBottom: '1rem', borderBottom: '1px solid var(--glass-border)', paddingBottom: '0.5rem' }}>
                            <div className="feature-label" style={{ fontSize: '1.1rem', color: 'var(--accent)' }}>
                                {getTranslation(feature.translations)}
                            </div>
                            <div className="badge" style={{ fontSize: '0.65rem' }}>
                                {feature.component_type}
                            </div>
                        </div>
                        <div className="feature-grid">
                            {feature.children.map((child: any) => <ConfigField key={child.feature_id} feature={child} />)}
                        </div>
                    </div>
                ) : (
                    <>
                        <div className="label-container">
                            <div className="feature-label">
                                {getTranslation(feature.translations)}
                            </div>
                            <div className="badge" style={{ color: 'var(--text-muted)', fontSize: '0.65rem' }}>
                                {feature.component_type}
                            </div>
                        </div>
                        <div className="control-container">
                            {renderInput()}
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
                        {data.features.map((feat: any) => <ConfigField key={feat.feature_id} feature={feat} />)}
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

    const renderSidebarItem = (svc: any, depth: number = 0) => {
        const hasChildren = svc.children && svc.children.length > 0;
        const isActive = selectedServiceId === svc.id;

        return (
            <div key={svc.id}>
                <div
                    className={`sidebar-item ${isActive ? 'active' : ''} ${depth > 0 ? 'nested' : ''}`}
                    onClick={() => setSelectedServiceId(svc.id)}
                    style={{
                        paddingLeft: `${1 + depth * 1.25}rem`,
                        fontSize: depth > 0 ? '0.9rem' : '1rem'
                    }}
                >
                    {depth === 0 ? getServiceIcon(svc.description_key) : <ChevronRight size={12} style={{ opacity: isActive ? 1 : 0.3 }} />}
                    <div style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
                        <span style={{ fontWeight: depth === 0 ? 600 : 400 }}>{getTranslation(svc.translations)}</span>
                    </div>
                </div>
                {hasChildren && (
                    <div className="sidebar-sub-group">
                        {svc.children.map((child: any) => renderSidebarItem(child, depth + 1))}
                    </div>
                )}
            </div>
        );
    };

    return (
        <div className="configurator-layout">
            {/* Side Menu */}
            <aside className="sidebar">
                <h2 className="section-title" style={{ padding: '0 1rem', marginBottom: '1.5rem', fontSize: '1.25rem', fontWeight: 800, letterSpacing: '-0.025em' }}>
                    Services
                </h2>
                <div className="sidebar-scroll">
                    {services.map((svc) => renderSidebarItem(svc))}
                </div>
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
