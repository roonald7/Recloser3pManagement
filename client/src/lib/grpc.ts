import * as grpc from '@grpc/grpc-js';
import * as protoLoader from '@grpc/proto-loader';
import path from 'path';

const PROTO_PATH = path.resolve(process.cwd(), '../proto/recloser.proto');

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true,
});

const protoDescriptor = grpc.loadPackageDefinition(packageDefinition) as any;
const recloserProto = protoDescriptor.recloser;

export const client = new recloserProto.RecloserService(
    process.env.GRPC_SERVER_ADDR || 'localhost:50051',
    grpc.credentials.createInsecure()
);

export const promisifyGrpc = <T>(method: any, request: any): Promise<T> => {
    return new Promise((resolve, reject) => {
        method.call(client, request, (error: any, response: T) => {
            if (error) {
                reject(error);
            } else {
                resolve(response);
            }
        });
    });
};
