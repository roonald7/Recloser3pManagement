import { getInventory, getServiceTree, getLanguages } from './actions';
import Configurator from './Configurator';
import { Cpu, ArrowLeft, ChevronRight, Server } from 'lucide-react';
import Link from 'next/link';

export default async function Page({
    searchParams,
}: {
    searchParams: { deviceId?: string; modelId?: string; firmwareId?: string; lang?: string };
}) {
    const deviceId = searchParams.deviceId ? parseInt(searchParams.deviceId) : null;
    const modelId = searchParams.modelId ? parseInt(searchParams.modelId) : null;
    const firmwareId = searchParams.firmwareId ? parseInt(searchParams.firmwareId) : null;
    const currentLang = searchParams.lang || 'enUs';

    const [devices, languages] = await Promise.all([
        getInventory(),
        getLanguages()
    ]);

    const getTranslation = (translations: any[], lang: string = currentLang) => {
        const t = translations?.find((t: any) => t.language_code === lang) || translations?.[0];
        return t?.value || 'N/A';
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

    // If device, model and firmware are selected, show the configurator
    if (deviceId && modelId && firmwareId) {
        const services = await getServiceTree(deviceId, modelId, firmwareId);
        const device = devices.find((d: any) => d.id === deviceId);
        const model = device?.models?.find((m: any) => m.id === modelId);
        const firmware = model?.firmwares?.find((f: any) => f.id === firmwareId);

        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flex: 1 }}>
                            <Link href={{ pathname: '/', query: { deviceId, modelId, lang: currentLang } }} className="back-button" style={{ marginBottom: 0 }}>
                                <ArrowLeft size={16} />
                            </Link>
                            <div>
                                <h1 style={{ fontSize: '1.75rem' }}>{getTranslation(device?.translations)} / {getTranslation(model?.translations)} / {getTranslation(firmware?.translations)}</h1>
                                <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem', marginTop: '0.25rem' }}>
                                    Configuration Dashboard
                                </p>
                            </div>
                        </div>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem' }}>
                            <LanguageSwitcher />
                            <div className="badge" style={{ borderColor: 'var(--primary)', color: 'var(--primary)' }}>
                                Online
                            </div>
                        </div>
                    </header>

                    <Configurator firmwareId={firmwareId} services={services} currentLang={currentLang} />
                </div>
            </main>
        );
    }

    // If device and model are selected, show firmware selection
    if (deviceId && modelId) {
        const device = devices.find((d: any) => d.id === deviceId);
        const model = device?.models?.find((m: any) => m.id === modelId);
        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flex: 1 }}>
                            <Link href={{ pathname: '/', query: { deviceId, lang: currentLang } }} className="back-button" style={{ marginBottom: 0 }}>
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
                                href={{ pathname: '/', query: { deviceId, modelId, firmwareId: fw.id, lang: currentLang } }}
                                className="card selection-card"
                            >
                                <Cpu size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                                <h3>Version {getTranslation(fw.translations)}</h3>
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

    // If only device is selected, show model selection
    if (deviceId) {
        const device = devices.find((d: any) => d.id === deviceId);
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
                        {device?.models?.map((md: any) => (
                            <Link
                                key={md.id}
                                href={{ pathname: '/', query: { deviceId, modelId: md.id, lang: currentLang } }}
                                className="card selection-card"
                            >
                                <Cpu size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                                <h3>{getTranslation(md.translations)}</h3>
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

    // Default: Show device selection
    return (
        <main>
            <div className="gradient-bg" />
            <div className="container">
                <header>
                    <div style={{ flex: 1 }}>
                        <h1>Device Management</h1>
                        <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
                            Select a hardware node to begin configuration
                        </p>
                    </div>
                    <LanguageSwitcher />
                </header>

                <div className="selector-grid">
                    {devices.map((device: any) => (
                        <Link
                            key={device.id}
                            href={{ pathname: '/', query: { deviceId: device.id, lang: currentLang } }}
                            className="card selection-card"
                        >
                            <Server size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                            <h3>{getTranslation(device.translations)}</h3>
                            <div style={{ marginTop: '1.5rem', color: 'var(--accent)' }}>
                                Open Device <ChevronRight size={14} />
                            </div>
                        </Link>
                    ))}

                    {devices.length === 0 && (
                        <div className="card" style={{ gridColumn: '1/-1', padding: '4rem', textAlign: 'center' }}>
                            <p style={{ color: 'var(--text-muted)' }}>No devices detected. Verify C++ server status.</p>
                        </div>
                    )}
                </div>
            </div>
        </main>
    );
}
