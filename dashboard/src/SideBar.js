import React from "react"
import HP from "./HPService"
import LedgerCardList from "./LedgerCardList"
import InputForm from "./InputForm"
import './SideBar.scss';

const nodeSorter = (a, b) => (a.idx > b.idx) ? 1 : ((b.idx > a.idx) ? -1 : 0);

class SideBar extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            region: null,
            nodeList: [],
            selectedNode: null,
            selectedIndex: 0
        }

        this.onNodeButtonClick = this.onNodeButtonClick.bind(this);
    }

    componentDidMount() {
        HP.nodeManager.on(HP.events.nodeSelected, (region, selectedNode) => {
            if (region !== this.state.region) {
                const nodes = Object.values(region.nodes);

                if (nodes.length > 1)
                    nodes.sort(nodeSorter);

                if (!selectedNode)
                    selectedNode = nodes[0];

                let selectedIndex = 0;
                nodes.forEach((n, i) => { if (n === selectedNode) selectedIndex = i; })

                this.setState({
                    region: region,
                    nodeList: nodes,
                    selectedNode: selectedNode,
                    selectedIndex: selectedIndex
                }, () => {
                    window.adjustLedgerScrollViewSize();
                });
            }
        });

        HP.nodeManager.on(HP.events.connectionStatusChanged, (node) => {
            if (this.state.region && this.state.region.name === node.name)
                this.setState(this.state);
        })

        HP.nodeManager.on(HP.events.syncStatusChanged, (node) => {
            if (this.state.region && this.state.region.name === node.name)
                this.setState(this.state);
        })

        HP.nodeManager.on(HP.events.regionListUpdated, (regionList) => {
            if (this.state.region && regionList[this.state.region.id]) {
                const nodes = Object.values(regionList[this.state.region.id].nodes);
                if (nodes.length > 1)
                    nodes.sort((a, b) => (a.idx > b.idx) ? 1 : ((b.idx > a.idx) ? -1 : 0));
                this.setState({
                    region: regionList[this.state.region.id],
                    nodeList: nodes,
                    selectedNode: nodes[0],
                    selectedIndex: 0
                })
            }
        });

    }

    onNodeButtonClick(node, iter) {
        const { selectedNode } = this.state;
        if (node !== selectedNode) {
            this.setState({
                ...this.state,
                selectedNode: node,
                selectedIndex: iter
            });
        }
    }

    render() {
        let { region, nodeList, selectedNode, selectedIndex } = this.state;

        let status = "initial";
        const nodeListButtons = nodeList.map((node, iter) => {
            const btnClass = (selectedNode === node) ? "btn-" : "btn-outline-";
            let status = "success";
            if (node.status === HP.connectionStatus.none)
                status = "danger";
            else if (node.isDesync)
                status = "warning";
            return <button className={"btn btn-sm rounded-pill " + btnClass + status + " m-1"}
                key={node.idx} onClick={() => this.onNodeButtonClick(node, iter)}>{iter + 1}</button>;
        });
        if (selectedNode) {

            if (selectedNode.status === HP.connectionStatus.none) {
                status = "error";
            } else if (selectedNode.isDesync) {
                status = "desync";
            } else {
                status = "normal";
            }
        }

        let statusText = "Connecting...";
        if (selectedNode) {

            if (selectedNode.status === HP.connectionStatus.none)
                statusText = "Not connected";
            else if (selectedNode.isDesync)
                statusText = "Out of sync";
            else
                statusText = "Connected";
        }

        const ip = selectedNode && selectedNode.host && selectedNode.host.split(":")[1].split("/")[2];

        return (
            <div className="d-flex flex-column m-md-2 pl-3 pr-3 pb-3 pt-1 pt-lg-3 sidebar">
                {selectedNode === null ?
                    <div>Please select a node.</div> :
                    <>
                        <div className="row">
                            <div className="col col-lg-12">
                                <h3 className="sidebar-node-title">
                                    {selectedNode.name} {(nodeListButtons.length > 1) && selectedIndex + 1}&nbsp;
                                    <span className={"d-none d-md-inline badge badge-pill status-" + status}>{statusText}</span>
                                </h3>
                            </div>
                            <div className="d-none d-lg-block col-12">
                                <div className="text-white-50 text-monospace mb-2">
                                    {ip} | id:{selectedNode.idx} | lcl:{selectedNode.lastLedger.seqNo}
                                </div>
                            </div>
                            <div className="col-auto col-lg-12">
                                {nodeListButtons.length > 1 &&
                                    <div className="node-num-bar mb-2">
                                        {nodeListButtons}
                                    </div>
                                }
                            </div>
                        </div>

                        <div className="flex">
                            <LedgerCardList node={selectedNode} />
                            {window.dashboardConfig.submitInputs && selectedNode.status === HP.connectionStatus.connected &&
                                (<>
                                    <InputForm node={selectedNode} region={region} idx={(nodeListButtons.length > 1) ? selectedIndex : -1} />
                                </>)}
                        </div>
                    </>
                }
            </div>
        )
    }
}

export default SideBar;