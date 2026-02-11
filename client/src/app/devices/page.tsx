import { getPhysicalDevices, getLanguages, getInventory } from '../actions';
import DeviceList from './DeviceList';
import { ArrowLeft, HardDrive } from 'lucide-react';
import Link from 'next/link';

export default async function DevicesPage({
    searchParams,
}: {
    searchParams: { lang?: string };
}) {
    const currentLang = searchParams.lang || 'enUs';

    const [physicalDevices, languages, inventory] = await Promise.all([
        getPhysicalDevices(),
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
                            <h1>Physical Device Fleet</h1>
                            <p style={{ color: 'var(--text-muted)', fontSize: '0.875rem', marginTop: '0.25rem' }}>
                                Manage and compare specific device instances and templates
                            </p>
                        </div>
                    </div>
                </header>

                <DeviceList
                    initialDevices={physicalDevices}
                    inventory={inventory}
                    currentLang={currentLang}
                />
            </div>
        </main>
    );
}
