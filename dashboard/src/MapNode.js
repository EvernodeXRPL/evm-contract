import React from "react"
import HP from "./HPService"
import LedgerCard from "./LedgerCard"
import "./MapNode.scss"

const emptyHash = "0000000000000000000000000000000000000000000000000000000000000000";

class MapNode extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            region: this.props.region,
            nodeList: Object.values(this.props.region.nodes)
        }

        this.onClick = this.onClick.bind(this);
    }

    componentDidMount() {

        const onUpdated = (node) => {
            this.setState(this.state)
        };

        HP.nodeManager.on(HP.events.connectionStatusChanged, onUpdated);
        HP.nodeManager.on(HP.events.syncStatusChanged, onUpdated);
        HP.nodeManager.on(HP.events.regionListUpdated, (regionList) => {
            if (this.state.region && regionList[this.state.region.id]) {
                const nodes = Object.values(regionList[this.state.region.id].nodes);
                if (nodes.length > 1)
                    nodes.sort((a, b) => { return (a.host < b.host) ? -1 : ((a.host > b.host) ? 1 : 0) });
                this.setState({
                    region: regionList[this.state.region.id],
                    nodeList: nodes
                })
            }
        });

        HP.nodeManager.on(HP.events.inputSubmissionUpdate, (node) => {
            this.setState({
                ...this.state
            });
        });
    }

    onClick() {
        const priorityNode = this.getPriorityNode();
        HP.nodeManager.selectNode(this.state.region, priorityNode.health() === 3 ? null : priorityNode);
    }

    getPriorityNode() {
        // Sort by health rating ascending.
        this.state.nodeList.sort((n1, n2) => {
            const a = n1.health();
            const b = n2.health();
            return (a > b) ? 1 : ((b > a) ? -1 : 0);
        })

        const priorityNode = this.state.nodeList[0];
        return priorityNode;
    }

    render() {

        const { region, nodeList } = this.state;
        const { pos } = region;
        const submittingInput = HP.nodeManager.inputSubmittingRegion === region;

        const priorityNode = this.getPriorityNode();
        const health = priorityNode.health();

        const status = health === 1 ? "error" : (health === 2 ? "desync" : "normal");

        let ledgerType = "";
        if (priorityNode.lastLedger && priorityNode.lastLedger.inputHash && priorityNode.lastLedger.inputHash !== emptyHash)
            ledgerType += " ledger-input";
        if (priorityNode.lastLedger && priorityNode.lastLedger.outputHash && priorityNode.lastLedger.outputHash !== emptyHash)
            ledgerType += " ledger-output";

        return (
            <div className={"map-node-container anchor-" + pos.anchor + " status-" + status + " " + ledgerType}
                style={{ top: pos.top, left: pos.left }}>
                <div className="map-ledger-card-container">
                    {nodeList.map((n, idx) => <LedgerCard onClick={() => this.onClick()}
                        key={idx} nodeName={region.name + " " + (idx + 1)} nodeHost={region.host} ledger={n.lastLedger} />)}
                </div>
                <div className="map-node-marker-container">

                    <div className="node-name-container">
                        <span className={"node-name badge badge-secondary p-1 anchor-" + pos.anchor + " " + (submittingInput ? "emphasize" : "")} onClick={() => this.onClick()}>
                            <span className="d-none d-lg-inline">{region.name} (</span>
                            <span className="seq-no">{priorityNode.lastLedger.seqNo}</span>
                            <span className="d-none d-lg-inline">)</span>
                        </span>
                    </div>
                    <div className={"map-node-marker rounded-circle " + (submittingInput ? "blink" : "")} onClick={() => this.onClick()}>
                        {nodeList.length}
                    </div>
                </div>
            </div>
        )
    }
}

export default MapNode;