import { getInventory, getServiceTree, getLanguages } from './actions';
import Configurator from './Configurator';
import { Cpu, ArrowLeft, ChevronRight, Server } from 'lucide-react';
import Link from 'next/link';

export default async function Page({
    searchParams,
}: {
    searchParams: { lineId?: string; modelId?: string; firmwareId?: string; lang?: string; deviceId?: string };
}) {
    const lineId = searchParams.lineId ? parseInt(searchParams.lineId) : null;
    const modelId = searchParams.modelId ? parseInt(searchParams.modelId) : null;
    const firmwareId = searchParams.firmwareId ? parseInt(searchParams.firmwareId) : null;
    const deviceId = searchParams.deviceId ? parseInt(searchParams.deviceId) : null;
    const currentLang = searchParams.lang || 'enUs';


    const [lines, languages] = await Promise.all([
        getInventory(),
        getLanguages()
    ]);

    const getTranslation = (data: any, lang: string = currentLang) => {
        const translations = data?.translations || data?.info?.translations;
        const t = translations?.find((t: any) => t.language_code === lang) || translations?.[0];
        return t?.value || data?.description_key || data?.info?.description_key || 'N/A';
    };

    const LanguageSwitcher = () => (
        <div style={{ display: 'flex', gap: '0.5rem' }}>
            {languages.map((lang: any) => (
                <Link
                    key={lang.code}
                    href={{
                        pathname: '/',
                        query: { ...searchParams, lang: lang.code }
                    }}
                    className={`badge`}
                    style={{
                        cursor: 'pointer',
                        background: currentLang === lang.code ? 'var(--primary)' : 'transparent',
                        color: currentLang === lang.code ? 'white' : 'var(--text-muted)',
                        borderColor: currentLang === lang.code ? 'var(--primary)' : 'var(--glass-border)',
                        transition: 'all 0.2s'
                    }}
                >
                    {lang.name}
                </Link>
            ))}
        </div>
    );

    // If line, model and firmware are selected, show the configurator
    if (lineId && modelId && firmwareId) {
        const services = await getServiceTree(lineId, modelId, firmwareId);
        const line = lines.find((d: any) => d.id === lineId);
        const model = line?.models?.find((m: any) => m.id === modelId);
        const firmware = model?.firmwares?.find((f: any) => f.id === firmwareId);

        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flex: 1 }}>
                            <Link href={{ pathname: '/', query: { lineId, modelId, lang: currentLang } }} className="back-button" style={{ marginBottom: 0 }}>
                                <ArrowLeft size={16} />
                            </Link>
                            <div>
                                <h1 style={{ fontSize: '1.75rem' }}>{getTranslation(line)} / {getTranslation(model)} / {getTranslation(firmware)}</h1>
                                <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem', marginTop: '0.25rem' }}>
                                    Configuration Dashboard
                                </p>
                            </div>
                        </div>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem' }}>
                            <LanguageSwitcher />
                        </div>

                    </header>

                    <Configurator lineId={lineId} modelId={modelId} firmwareId={firmwareId} deviceId={deviceId || undefined} services={services} currentLang={currentLang} />

                </div>
            </main>
        );
    }

    // If line and model are selected, show firmware selection
    if (lineId && modelId) {
        const line = lines.find((d: any) => d.id === lineId);
        const model = line?.models?.find((m: any) => m.id === modelId);
        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flex: 1 }}>
                            <Link href={{ pathname: '/', query: { lineId, lang: currentLang } }} className="back-button" style={{ marginBottom: 0 }}>
                                <ArrowLeft size={16} />
                            </Link>
                            <h1>Select Firmware Version</h1>
                        </div>
                        <LanguageSwitcher />
                    </header>

                    <div className="selector-grid">
                        {model?.firmwares?.map((fw: any) => (
                            <Link
                                key={fw.id}
                                href={{ pathname: '/', query: { lineId, modelId, firmwareId: fw.id, lang: currentLang } }}
                                className="card selection-card"
                            >
                                <Cpu size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                                <h3>Version {getTranslation(fw)}</h3>
                                <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
                                    {fw.services?.length || 0} top-level services
                                </p>
                                <div style={{ marginTop: '1.5rem', color: 'var(--accent)' }}>
                                    Configure <ChevronRight size={14} />
                                </div>
                            </Link>
                        ))}
                    </div>
                </div>
            </main>
        );
    }

    // If only line is selected, show model selection
    if (lineId) {
        const line = lines.find((d: any) => d.id === lineId);
        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flex: 1 }}>
                            <Link href={{ pathname: '/', query: { lang: currentLang } }} className="back-button" style={{ marginBottom: 0 }}>
                                <ArrowLeft size={16} />
                            </Link>
                            <h1>Select Hardware Model</h1>
                        </div>
                        <LanguageSwitcher />
                    </header>

                    <div className="selector-grid">
                        {line?.models?.map((md: any) => (
                            <Link
                                key={md.id}
                                href={{ pathname: '/', query: { lineId, modelId: md.id, lang: currentLang } }}
                                className="card selection-card"
                            >
                                <Cpu size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                                <h3>{getTranslation(md)}</h3>
                                <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
                                    {md.firmwares?.length || 0} firmware versions
                                </p>
                                <div style={{ marginTop: '1.5rem', color: 'var(--accent)' }}>
                                    Select Model <ChevronRight size={14} />
                                </div>
                            </Link>
                        ))}
                    </div>
                </div>
            </main>
        );
    }

    // Default: Show line selection
    return (
        <main>
            <div className="gradient-bg" />
            <div className="container">
                <header>
                    <div style={{ flex: 1 }}>
                        <h1>Management Dashboard</h1>
                        <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
                            Select a hardware line to begin configuration
                        </p>
                    </div>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem' }}>
                        <Link
                            href={{ pathname: '/lines', query: { lang: currentLang } }}
                            className="badge"
                            style={{
                                background: 'var(--glass)',
                                color: 'var(--accent)',
                                borderColor: 'var(--accent)',
                                textDecoration: 'none',
                                display: 'flex',
                                alignItems: 'center',
                                gap: '0.5rem',
                                padding: '0.5rem 1rem'
                            }}
                        >
                            <Server size={14} /> View Fleet & Templates
                        </Link>
                        <LanguageSwitcher />
                    </div>
                </header>


                <div className="selector-grid">
                    {lines.map((line: any) => (
                        <Link
                            key={line.id}
                            href={{ pathname: '/', query: { lineId: line.id, lang: currentLang } }}
                            className="card selection-card"
                        >
                            <Server size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                            <h3>{getTranslation(line)}</h3>
                            <div style={{ marginTop: '1.5rem', color: 'var(--accent)' }}>
                                Open Line <ChevronRight size={14} />
                            </div>
                        </Link>
                    ))}

                    {lines.length === 0 && (
                        <div className="card" style={{ gridColumn: '1/-1', padding: '4rem', textAlign: 'center' }}>
                            <p style={{ color: 'var(--text-muted)' }}>No hardware lines detected. Verify C++ server status.</p>
                        </div>
                    )}
                </div>
            </div>
        </main>
    );
}
