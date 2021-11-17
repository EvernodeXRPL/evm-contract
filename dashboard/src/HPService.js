import EventEmitter from "EventEmitter"
const signalR = require("@microsoft/signalr");
const { TableClient, AzureSASCredential } = require("@azure/data-tables");

// const HotPocket = window.HotPocket;
// HotPocket.setLogLevel(1);

const emptyHash = "0000000000000000000000000000000000000000000000000000000000000000";

const connectionStatus = {
    none: 0,
    connected: 1
}

const events = {
    connectionStatusChanged: "connectionStatusChanged",
    syncStatusChanged: "syncStatusChanged",
    nodeSelected: "nodeSelected",
    ledgerUpdated: "ledgerUpdated",
    inputSubmissionUpdate: "inputSubmissionUpdate",
    regionListUpdated: "regionListUpdated"
}

const maxLedgers = 20; // Max ledgers to keep per node.
const hpRegions = {};

class HPNode {
    constructor(name, host, idx, pos, nodeManager) {
        this.name = name;
        this.host = host;
        this.idx = idx;
        this.pos = pos;
        this.nodeManager = nodeManager;

        this.status = connectionStatus.none;
        this.emitter = new EventEmitter();
        this.ledgers = [];

        this.reset();
    }

    health() {
        let rating = 3;
        if (this.isDesync)
            rating = 2;
        else if (this.status === connectionStatus.none)
            rating = 1;

        return rating; // Higher rating means good.
    }

    reset() {
        this.inputSubmission = {
            inProgress: false,
            lastHash: null,
            failureReason: null,
            ledgerSeqNo: null
        }
    }

    on(event, handler) {
        this.emitter.on(event, handler);
    }

    off(event, handler) {
        this.emitter.off(event, handler);
    }

    updateConnectionStatus(status) {
        this.status = status;
        this.nodeManager.emitter.emit(events.connectionStatusChanged, this);
    }

    onLedgerEvent(ev) {

        if (ev.event === "ledger_created") {
            this.updateConnectionStatus(connectionStatus.connected);

            // Convert inputs and outputs to simple strings for display purposes.
            // Input and output blobs from other user's will be empty because if the cluster consensus is private.
            const ledger = ev.ledger;
            ledger.inputs = visualizeInputs(ledger);
            ledger.outputs = visualizeOutputs(ledger);

            this.lastLedger = ledger;
            this.ledgers.push(ledger);

            if (this.ledgers.length > maxLedgers)
                this.ledgers.shift(); // Remove first (oldest) ledger in the array.

            this.nodeManager.emitter.emit(events.ledgerUpdated, this);

            // Creating a ledger means this node is in sync.
            if (this.isDesync) {
                this.isDesync = false;
                this.nodeManager.emitter.emit(events.syncStatusChanged, this)
            }
        }
        else if (ev.event === "vote_status") {
            this.isDesync = (ev.voteStatus === "desync");
            this.updateConnectionStatus(connectionStatus.connected);
            this.nodeManager.emitter.emit(events.syncStatusChanged, this);
        } else if (ev.event === "online") {
            this.isDesync = true;
            this.updateConnectionStatus(connectionStatus.connected);
            this.nodeManager.emitter.emit(events.syncStatusChanged, this);
        }
        else if (ev.event === "offline") {
            this.updateConnectionStatus(connectionStatus.none);
        }
    }
}

class HPNodeManager {

    constructor() {
        this.keys = null;
        this.emitter = new EventEmitter();
        this.signalRConnection = null;
        this.inputSubmittingRegion = null;
        this.inputSubmittingNode = null;
    }

    async start() {
        await this.loadLastNodeState();
        this.connectToSignalR(); // Connect to signalr asynchronously.

        // this.addNode({
        //     idx: 1,
        //     uri: "wss://localhost:8081",
        //     isDesync: false,
        //     lastLedger: {
        //         seqNo: 1001,
        //         inputHash: "something",
        //         outputHash: "something"
        //     },
        //     connectionStatus: connectionStatus.connected
        // });
        // this.emitter.emit(events.regionListUpdated, hpRegions);

        // Select a random good node on initial load.
        const goodNodes = [];
        for (const region of Object.values(hpRegions)) {
            if (region.nodes) {
                const regionGoodNodes = Object.values(region.nodes).filter(n => n.health() === 3);
                goodNodes.push(...regionGoodNodes.map(n => { return { region: region, node: n } }));
            }
        }
        if (goodNodes.length > 0) {
            const random = goodNodes[Math.trunc(goodNodes.length * Math.random())];
            this.selectNode(random.region, random.node);
        }
    }

    async loadLastNodeState() {
        // Load last known status of all nodes from table stoage.

        const tableClient = new TableClient(
            window.dashboardConfig.tableAccount,
            window.dashboardConfig.tableName,
            new AzureSASCredential(window.dashboardConfig.tableSas));

        const rows = await tableClient.listEntities({ queryOptions: { filter: `PartitionKey eq '${window.dashboardConfig.clusterKey}'` } })
        let isListUpdated = false;
        for await (const row of rows) {
            if (this.addNode({
                idx: parseInt(row.rowKey),
                uri: row.Uri,
                lastLedger: JSON.parse(row.LastLedger),
                connectionStatus: (row.Status === "offline" ? connectionStatus.none : connectionStatus.connected),
                isDesync: row.Status === "desync"
            })) {
                isListUpdated = true;
            }

        }
        if (isListUpdated)
            this.emitter.emit(events.regionListUpdated, hpRegions);
    }

    async connectToSignalR() {
        try {
            this.signalRConnection = new signalR.HubConnectionBuilder()
                .withUrl(window.dashboardConfig.signalRUrl, {
                    headers: { "x-ms-client-principal-id": window.dashboardConfig.clusterKey }
                })
                .configureLogging(signalR.LogLevel.Information)
                .withAutomaticReconnect()
                .build();

            this.signalRConnection.on("newMessage", (message) => {
                let isListUpdated = false;
                message.forEach(msg => {

                    if (this.addNode(msg))
                        isListUpdated = true;
                });
                if (isListUpdated)
                    this.emitter.emit(events.regionListUpdated, hpRegions);
            });

            await this.signalRConnection.start();
        } catch (error) {
            console.error(error);
        }
    }

    addNode(msg) {
        let isListUpdated = false;
        let region = null;

        // Check whether there's a special region assignment for this node index.
        const specialAssignment = window.dashboardConfig.specialRegionAssignments.filter(a => a.idx === msg.idx)[0];
        if (specialAssignment) {
            region = window.dashboardConfig.regions.filter(r => r.id === specialAssignment.regionId)[0];
            if (!region)
                return;
        }
        else {
            const cycleRegions = window.dashboardConfig.regions.filter(r => r.skipCycling !== true);
            const regionIndex = (msg.idx - 1) % cycleRegions.length;
            region = cycleRegions[regionIndex];
        }

        if (!hpRegions[region.id]) {
            hpRegions[region.id] = region;
            hpRegions[region.id].nodes = {};
            hpRegions[region.id].nodes[msg.uri] = new HPNode(region.name, msg.uri, msg.idx, region.pos, exports.nodeManager);
            isListUpdated = true;
        } else if (!hpRegions[region.id].nodes[msg.uri]) {
            isListUpdated = true;
            hpRegions[region.id].nodes[msg.uri] = new HPNode(region.name, msg.uri, msg.idx, region.pos, exports.nodeManager);
        }

        const node = hpRegions[region.id].nodes[msg.uri];

        if (msg.data)
            node.onLedgerEvent(msg.data);

        if (msg.lastLedger)
            node.lastLedger = msg.lastLedger;

        if (msg.connectionStatus != null)
            node.status = msg.connectionStatus;

        if (msg.isDesync)
            node.isDesync = msg.isDesync;

        return isListUpdated;
    }

    on(event, handler) {
        this.emitter.on(event, handler);
    }

    off(event, handler) {
        this.emitter.off(event, handler);
    }

    selectNode(region, node) {
        this.emitter.emit(events.nodeSelected, region, node);
    }

    async submitInput(region, node, input) {

        if (this.inputSubmittingNode || !input || input.length === 0)
            return;

        this.inputSubmittingRegion = region;
        this.inputSubmittingNode = node;
        node.inputSubmission.inProgress = true;

        this.emitter.emit(events.inputSubmissionUpdate, node);

        // const hpClient = await HotPocket.createClient([node.host], this.keys);
        // if (await hpClient.connect()) {
        //     const hpInput = await hpClient.submitContractInput(input);
        //     node.inputSubmission.lastHash = hpInput.hash;
        //     this.emitter.emit(events.inputSubmissionUpdate, node);

        //     const submission = await hpInput.submissionStatus;
        //     hpClient.close()

        //     node.inputSubmission.failureReason = submission.status !== "accepted" ? `Failed: ${submission.reason}` : null;
        //     node.inputSubmission.ledgerSeqNo = submission.ledgerSeqNo;
        // }

        if (true) { // Use 'false' for testing.
            const funcUrl = `${window.dashboardConfig.inputFunc}&uri=${node.host}&input=${input}&output=1`;
            const resp = await fetch(funcUrl, { method: 'POST' });
            const obj = await resp.json();
            node.inputSubmission.lastHash = obj.inputHash;
            node.inputSubmission.failureReason = obj.failureReason;
            node.inputSubmission.ledgerSeqNo = obj.ledgerSeqNo;
            node.inputSubmission.output = obj.output;
        }
        else {
            await new Promise(resolve => setTimeout(() => {
                resolve();
            }, 1000));
            node.inputSubmission.lastHash = "testhash";
            node.inputSubmission.ledgerSeqNo = 100;
            node.inputSubmission.output = "ecall_error";
        }

        const code = node.inputSubmission.output && node.inputSubmission.output.substr(0, 1);
        node.inputSubmission.output = node.inputSubmission.output && node.inputSubmission.output.substr(1);

        if (code === "d")
            node.inputSubmission.output = "Deployed the bytecode on account " + node.inputSubmission.output;

        node.inputSubmission.inProgress = false;
        this.inputSubmittingRegion = null;
        this.inputSubmittingNode = null;
        this.emitter.emit(events.inputSubmissionUpdate, node);
    }
}

function visualizeInputs(ledger) {
    if (window.dashboardConfig.rawDataMode === "text") {
        return ledger.inputs ? ledger.inputs.filter(i => i.blob.length > 0).map(i => hexToString(i.blob)) : [];
    }
    else if (window.dashboardConfig.rawDataMode === "len")
        return ledger.inputs ? ledger.inputs.filter(i => i.blob.length > 0).map(i => `len:${i.blob.length / 2}`) : [];
    else
        return (ledger.inputHash !== emptyHash) ? [ledger.inputHash.substr(0, 10) + "..."] : []
}

function visualizeOutputs(ledger) {
    if (window.dashboardConfig.rawDataMode === "text")
        return ledger.outputs ? [].concat.apply([], ledger.outputs.filter(o => o.blobs.length > 0).map(o => o.blobs.map(b => hexToString(b)))) : [];
    else if (window.dashboardConfig.rawDataMode === "len")
        return ledger.outputs ? [].concat.apply([], ledger.outputs.filter(o => o.blobs.length > 0).map(o => o.blobs.map(b => `len:${b.length / 2}`))) : [];
    else
        return (ledger.outputHash !== emptyHash) ? [ledger.outputHash.substr(0, 10) + "..."] : []
}

function hexToString(hexString) {
    const arr = new Uint8Array(hexString.match(/.{1,2}/g).map(byte => parseInt(byte, 16)));
    const str = new TextDecoder().decode(arr);
    return str;
}

const exports = {
    events: events,
    nodeManager: new HPNodeManager(),
    connectionStatus: connectionStatus
}

export default exports