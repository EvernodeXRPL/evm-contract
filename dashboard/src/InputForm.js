import React from "react"
import HP from "./HPService"
import './InputForm.scss';

class InputForm extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            input: "",
            addr: "",
            mode: "deploy"
        }

        this.handleModeChange = this.handleModeChange.bind(this);
        this.handleAddressChange = this.handleAddressChange.bind(this);
        this.handleInputdataChange = this.handleInputdataChange.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
        this.isValidAddress = this.isValidAddress.bind(this);
        this.isValidInputdata = this.isValidInputdata.bind(this);
    }

    componentDidMount() {
        HP.nodeManager.on(HP.events.inputSubmissionUpdate, (node) => {

            // Update our state if we are currently displaying the notified node.
            if (this.props.node === node) {
                this.setState(this.state);
            }
        });
    }

    handleModeChange(event) {
        this.setState({ ...this.state, mode: event.target.value });
    }

    handleAddressChange(event) {
        this.setState({ ...this.state, addr: event.target.value });
    }

    handleInputdataChange(event) {
        const input = event.target.value.replace(/\n|\r/g, "");
        this.setState({ ...this.state, input: input });
    }

    isValidAddress() {

        // In deploy we allow blank address since we auto generate if blank.
        if (this.state.mode === "deploy" && this.state.addr.length === 0)
            return true;

        return isHex(this.state.addr, 40);
    }

    isValidInputdata() {
        if (this.state.input === 0)
            return false;
        return isHex(this.state.input);
    }

    async handleSubmit(event) {
        event.preventDefault();

        const deploy = this.state.mode === "deploy";
        let addr = this.state.addr;
        if (deploy && addr.length === 0) {
            addr = newAddressHex();
            this.state.addr = addr;
        }

        const input = (deploy ? "d" : "c") + stripHex(addr) + stripHex(this.state.input);
        HP.nodeManager.submitInput(this.props.region, this.props.node, input);
    }

    render() {
        const { node, idx } = this.props;
        const deploy = this.state.mode === "deploy";
        const submitting = HP.nodeManager.inputSubmittingNode != null;
        const canSubmit = this.isValidAddress() && this.isValidInputdata();
        const addressTip = deploy ?
            "The ETH address to deploy the contract. If blank, we'll generate an address for you." :
            "The ETH address of the contract you want to call.";

        const errorTip = deploy ?
            "Address and bytecode must be in hex format." :
            "Address and call input must be in hex format.";

        return (
            <div className={"input-form mt-4" + (submitting ? " submitting" : "")}>
                <form className="mb-2" onSubmit={this.handleSubmit}>
                    <div className="text-center" onChange={this.handleModeChange}>
                        <label className="pr-2">
                            <input type="radio" name="mode" value="deploy" defaultChecked /> Deploy
                        </label>
                        <label>
                            <input type="radio" name="mode" value="call" /> Call
                        </label>
                    </div>
                    <div className="input-group input-group-sm mb-1" title={addressTip}>
                        <div className="input-group-prepend">
                            <span className="input-group-text">Address</span>
                        </div>
                        <input type="text" className="form-control text-monospace" maxLength="42" disabled={submitting} placeholder="Account address..." value={this.state.addr} onChange={this.handleAddressChange} />
                    </div>
                    <textarea className="inputdata form-control text-monospace p-1 mb-2" disabled={submitting} placeholder={deploy ? "Bytecode..." : "Call input data..."} value={this.state.input} onChange={this.handleInputdataChange} />
                    <div className="row">
                        <div className="col">
                            {canSubmit ?
                                <span>Your are submitting to {node.name} {idx !== -1 && idx + 1} node.</span> :
                                <span className="text-warning">{errorTip}</span>}
                        </div>
                        <div className="col-auto">
                            <button className="btn btn-primary btn-sm pl-3 pr-3" type="submit" disabled={submitting || !canSubmit}>
                                {deploy ? "Deploy" : "Call"}
                            </button>
                        </div>
                    </div>
                </form>

                {submitting ?
                    <div>
                        <h6>Submitting... <div className="d-inline-block spinner"></div></h6>
                        {node.inputSubmission.lastHash &&
                            <div className="text-truncate"><strong>Hash:</strong> {node.inputSubmission.lastHash}</div>}
                    </div> :
                    node.inputSubmission.lastHash &&
                    <div>
                        <h6>Last submission result :</h6>
                        {/* <div className="text-truncate"><strong>Hash:</strong> {node.inputSubmission.lastHash}</div> */}
                        {node.inputSubmission.ledgerSeqNo && <div><strong>Ledger :</strong> {node.inputSubmission.ledgerSeqNo}</div>}
                        {node.inputSubmission.failureReason && <div><strong>Failed :</strong> {node.inputSubmission.failureReason}</div>}
                        {node.inputSubmission.output &&
                            <div>
                                <textarea className="responsedata form-control text-monospace p-1 mt-2" readOnly={true} value={node.inputSubmission.output} />
                            </div>}
                    </div>
                }


            </div>
        )
    }
}

function newAddressHex() {
    return "0x" + Array.from(crypto.getRandomValues(new Uint8Array(20)))
        .map(b => b.toString(16).padStart(2, '0')).join('');
}

function stripHex(str) {
    str = str.toLowerCase();
    if (str.startsWith("0x"))
        str = str.substr(2);
    return str;
}

function isHex(str, len) {
    str = stripHex(str);
    if (len && str.length !== len)
        return false;

    return str.match("^[0-9a-fA-F]+$");
}

export default InputForm;