import { getDevices, getLanguages, getInventory } from '../actions';
import LineList from './LineList';
import { ArrowLeft, HardDrive } from 'lucide-react';
import Link from 'next/link';

export default async function DevicesPage({
    searchParams,
}: {
    searchParams: { lang?: string };
}) {
    const currentLang = searchParams.lang || 'enUs';

    const [devices, languages, inventory] = await Promise.all([
        getDevices(),
        getLanguages(),
        getInventory()
    ]);

    return (
        <main>
            <div className="gradient-bg" />
            <div className="container">
                <header>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '1.5rem', flex: 1 }}>
                        <Link href={{ pathname: '/', query: { lang: currentLang } }} className="back-button" style={{ marginBottom: 0 }}>
                            <ArrowLeft size={16} />
                        </Link>
                        <div>
                            <h1>Device Fleet</h1>
                            <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem', marginTop: '0.25rem' }}>
                                Manage and compare specific device instances and templates
                            </p>
                        </div>
                    </div>
                </header>

                <LineList
                    initialDevices={devices}
                    inventory={inventory}
                    currentLang={currentLang}
                />
            </div>
        </main>
    );
}
