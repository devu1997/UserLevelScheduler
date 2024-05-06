const fs = require('fs').promises;

// ---------------------------
// CPU Tasks

async function generatePrimes(limit) {
    let isPrime = new Array(limit + 1).fill(true);
    let primes = [];

    for (let p = 2; p * p <= limit; ++p) {
        if (isPrime[p]) {
            for (let i = p * p; i <= limit; i += p) {
                isPrime[i] = false;
            }
        }
    }

    for (let p = 2; p <= limit; ++p) {
        if (isPrime[p]) {
            primes.push(p);
        }
    }

    return primes;
}

async function generateCpuTaskChain(length) {

    let new_length = length * 2

    for (let i = 0; i < new_length - 1; i++) {
        let a = await generatePrimes(10000);
    }
    return 'Done'
}

// ---------------------------
// IO Tasks

async function openReadFile(filePath) {
    try {
        const fileHandle = await fs.open(filePath, 'r', fs.constants.O_RDONLY | fs.constants.O_DIRECT | fs.constants.O_SYNC);
        return fileHandle;
    } catch (error) {
        throw new Error(`Error opening read file: ${error}`);
    }
}

async function readToBuffer(fileHandle) {
    try {
        const buffer = await fileHandle.readFile();
        return buffer;
    } catch (error) {
        throw new Error(`Error reading from file: ${error}`);
    }
}

async function closeReadFile(fileHandle) {
    try {
        await fileHandle.close();
    } catch (error) {
        throw new Error(`Error closing read file: ${error}`);
    }
}

async function openWriteFile(filePath) {
    try {
        const fileHandle = await fs.open(filePath, 'w', fs.constants.O_RDWR | fs.constants.O_CREAT | fs.constants.O_TRUNC | fs.constants.O_DIRECT | fs.constants.O_SYNC);
        return fileHandle;
    } catch (error) {
        throw new Error(`Error opening write file: ${error}`);
    }
}

async function writeToFile(fileHandle, data) {
    try {
        await fileHandle.writeFile(data);
    } catch (error) {
        throw new Error(`Error writing to file: ${error}`);
    }
}

async function closeWriteFile(fileHandle) {
    try {
        await fileHandle.close();
    } catch (error) {
        throw new Error(`Error closing write file: ${error}`);
    }
}

async function generateIoTaskChain(sourceFilePath, destinationFilePath) {
    let readHandle, writeHandle;

    try {
        // Open the read file
        readHandle = await openReadFile(sourceFilePath);

        // Read file to buffer
        const buffer = await readToBuffer(readHandle);

        // Close the read file
        await closeReadFile(readHandle);

        // Open the write file
        writeHandle = await openWriteFile(destinationFilePath);

        // Write buffer to file
        for (let k = 0; k < 20; k++) {
            await writeToFile(writeHandle, buffer);
        }

        // Close the write file
        await closeWriteFile(writeHandle);

    } catch (error) {
        console.error(error);
    }
}

// ---------------------------
// Benchmark functions

async function multipleSchedulerMultiIoCpuTasksBenchmark() {
    const start_time = new Date();
    let all_tasks = []

    for (let i = 0; i < 256; i++) {
        all_tasks.push(generateIoTaskChain('logs/log.txt', 'logs/log'+i+'.txt'));
    }
    for (let i = 0; i < 256; i++) {
        all_tasks.push(generateCpuTaskChain(20*20));
    }

    Promise.all(all_tasks).then(e => console.log('All tasks finished in : ', new Date() - start_time, ' milliseconds.'));
}

async function multipleSchedulerMultiCpuCpuTasksBenchmark() {
    const start_time = new Date();
    let all_tasks = []

    for (let i = 0; i < 512; i++) {
        all_tasks.push(generateCpuTaskChain(20*20));
    }

    Promise.all(all_tasks).then(e => console.log('All tasks finished in : ', new Date() - start_time, ' milliseconds.'));
}

async function multipleSchedulerMultiIoIoTasksBenchmark() {
    const start_time = new Date();
    let all_tasks = []

    for (let i = 0; i < 512; i++) {
        all_tasks.push(generateIoTaskChain('logs/log.txt', 'logs/log'+i+'.txt'));
    }

    Promise.all(all_tasks).then(e => console.log('All tasks finished in : ', new Date() - start_time, ' milliseconds.'));
}

// multipleSchedulerMultiIoCpuTasksBenchmark();
// multipleSchedulerMultiCpuCpuTasksBenchmark();
multipleSchedulerMultiIoIoTasksBenchmark();
