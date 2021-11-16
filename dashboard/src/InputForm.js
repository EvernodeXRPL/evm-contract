import React from "react"
import HP from "./HPService"
import './InputForm.scss';

class InputForm extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            input: ""
        }

        this.handleChange = this.handleChange.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
    }

    componentDidMount() {
        HP.nodeManager.on(HP.events.inputSubmissionUpdate, (node) => {

            // Update our state if we are currently displaying the notified node.
            if (this.props.node === node)
                this.setState(this.state)
        });
    }

    handleChange(event) {
        const input = event.target.value.slice(0, 100);
        this.setState({ ...this.state, input: input });
    }

    async handleSubmit(event) {
        event.preventDefault();
        HP.nodeManager.submitInput(this.props.region, this.props.node, this.state.input);
        this.setState({ ...this.state, input: "" });
    }

    render() {
        const { node, idx } = this.props;
        const submitting = HP.nodeManager.inputSubmittingNode != null;

        return (
            <div className={"input-form mt-2" + (submitting ? " submitting" : "")}>
                <form className="mb-2" onSubmit={this.handleSubmit}>
                    <div className="input-group input-group-sm">
                        <input type="text" className="form-control" disabled={submitting} placeholder="Your input..." value={this.state.input} onChange={this.handleChange} />
                        <div className="input-group-append">
                            <button className="btn btn-primary" type="submit" disabled={submitting || this.state.input.length === 0}>Submit</button>
                        </div>
                    </div>
                    <div className="pt-1">
                        Your input will be submitted to {node.name} {idx !== -1 && idx + 1} node.
                    </div>
                </form>

                {submitting ?
                    <div>
                        <h6>Submitting input... <div className="d-inline-block spinner"></div></h6>
                        {node.inputSubmission.lastHash &&
                            <div className="text-truncate"><strong>Hash:</strong> {node.inputSubmission.lastHash}</div>}
                    </div> :
                    node.inputSubmission.lastHash &&
                    <div>
                        <h6>Last submission</h6>
                        {/* <div className="text-truncate"><strong>Hash:</strong> {node.inputSubmission.lastHash}</div> */}
                        {node.inputSubmission.ledgerSeqNo && <div><strong>Ledger:</strong> {node.inputSubmission.ledgerSeqNo}</div>}
                        {node.inputSubmission.failureReason && <div><strong>Failed:</strong> {node.inputSubmission.failureReason}</div>}
                    </div>
                }


            </div>
        )
    }
}

export default InputForm;