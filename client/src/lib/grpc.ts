import * as grpc from '@grpc/grpc-js';
import * as protoLoader from '@grpc/proto-loader';
import path from 'path';

const PROTO_PATH = path.resolve(process.cwd(), '../proto/device.proto');

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true,
});

const protoDescriptor = grpc.loadPackageDefinition(packageDefinition) as any;
const deviceProto = protoDescriptor.device;

export const client = new deviceProto.DeviceService(
    process.env.GRPC_SERVER_ADDR || 'localhost:50051',
    grpc.credentials.createInsecure()
);

export const recordClient = new deviceProto.RecordService(
    process.env.GRPC_SERVER_ADDR || 'localhost:50051',
    grpc.credentials.createInsecure()
);

export const promisifyGrpc = <T>(cli: any, method: any, request: any): Promise<T> => {
    return new Promise((resolve, reject) => {
        method.call(cli, request, (error: any, response: T) => {
            if (error) {
                reject(error);
            } else {
                resolve(response);
            }
        });
    });
};
