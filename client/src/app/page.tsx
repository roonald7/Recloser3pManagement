import { getInventory, getServiceTree } from './actions';
import Configurator from './Configurator';
import { Cpu, ArrowLeft, ChevronRight, Server } from 'lucide-react';
import Link from 'next/link';

export default async function Page({
    searchParams,
}: {
    searchParams: { recloserId?: string; firmwareId?: string };
}) {
    const recloserId = searchParams.recloserId ? parseInt(searchParams.recloserId) : null;
    const firmwareId = searchParams.firmwareId ? parseInt(searchParams.firmwareId) : null;

    const reclosers = await getInventory();

    const getTranslation = (translations: any[], lang: string = 'enUs') => {
        const t = translations?.find((t: any) => t.language_code === lang) || translations?.[0];
        return t?.value || 'N/A';
    };

    // If both recloser and firmware are selected, show the configurator
    if (recloserId && firmwareId) {
        const services = await getServiceTree(firmwareId);
        const recloser = reclosers.find((r: any) => r.id === recloserId);
        const firmware = recloser?.firmwares?.find((f: any) => f.id === firmwareId);

        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem' }}>
                            <Link href="/" className="back-button" style={{ marginBottom: 0 }}>
                                <ArrowLeft size={16} />
                            </Link>
                            <div>
                                <div className="badge" style={{ marginBottom: '0.5rem', display: 'inline-block', borderColor: 'var(--primary)' }}>
                                    {recloser?.description_key}
                                </div>
                                <h1 style={{ fontSize: '1.75rem' }}>{getTranslation(recloser?.translations)} / {firmware?.version}</h1>
                                <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem', marginTop: '0.25rem' }}>
                                    Configuration Dashboard
                                </p>
                            </div>
                        </div>
                        <div className="badge" style={{ borderColor: 'var(--primary)', color: 'var(--primary)' }}>
                            Online
                        </div>
                    </header>

                    <Configurator firmwareId={firmwareId} services={services} />
                </div>
            </main>
        );
    }

    // If only recloser is selected, show firmware selection
    if (recloserId) {
        const recloser = reclosers.find((r: any) => r.id === recloserId);
        return (
            <main>
                <div className="gradient-bg" />
                <div className="container">
                    <header>
                        <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem' }}>
                            <Link href="/" className="back-button" style={{ marginBottom: 0 }}>
                                <ArrowLeft size={16} />
                            </Link>
                            <h1>Select Firmware Version</h1>
                        </div>
                    </header>

                    <div className="selector-grid">
                        {recloser?.firmwares?.map((fw: any) => (
                            <Link
                                key={fw.id}
                                href={`/?recloserId=${recloserId}&firmwareId=${fw.id}`}
                                className="card selection-card"
                            >
                                <Cpu size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                                <h3>Version {fw.version}</h3>
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

    // Default: Show recloser selection
    return (
        <main>
            <div className="gradient-bg" />
            <div className="container">
                <header>
                    <div>
                        <h1>Recloser Management</h1>
                        <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
                            Select a hardware node to begin configuration
                        </p>
                    </div>
                </header>

                <div className="selector-grid">
                    {reclosers.map((recloser: any) => (
                        <Link
                            key={recloser.id}
                            href={`/?recloserId=${recloser.id}`}
                            className="card selection-card"
                        >
                            <Server size={48} color="var(--primary)" style={{ marginBottom: '1rem' }} />
                            <h3>{getTranslation(recloser.translations)}</h3>
                            <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem' }}>
                                {recloser.description_key}
                            </p>
                            <div style={{ marginTop: '1.5rem', color: 'var(--accent)' }}>
                                Open Device <ChevronRight size={14} />
                            </div>
                        </Link>
                    ))}

                    {reclosers.length === 0 && (
                        <div className="card" style={{ gridColumn: '1/-1', padding: '4rem', textAlign: 'center' }}>
                            <p style={{ color: 'var(--text-muted)' }}>No reclosers detected. Verify C++ server status.</p>
                        </div>
                    )}
                </div>
            </div>
        </main>
    );
}
