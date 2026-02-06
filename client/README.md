# Recloser Management Client

This is a premium Next.js dashboard designed to interact with the Recloser Management C++ Service via gRPC.

## Prerequisites

- **Node.js**: v18.17+ recommended.
- **C++ Server**: The Recloser Management System server must be running (usually on port 50051).

## Getting Started

1. **Install dependencies**:

   ```bash
   npm install
   ```

2. **Configure connection**:
   Update `.env.local` if your gRPC server is on a different address:

   ```env
   GRPC_SERVER_ADDR=localhost:50051
   ```

3. **Run the dashboard**:

   ```bash
   npm run dev
   ```

4. **Open in browser**:
   Navigate to [http://localhost:3000](http://localhost:3000).

## Features

- **Device Inventory**: View all recloser models and their associated translations.
- **Firmware Tracking**: Explore firmware versions available for each device.
- **Service Explorer**: High-performance tree visualization of hardware services.
- **Modern Aesthetics**: Glassmorphism UI with dark mode optimized for technical dashboards.
