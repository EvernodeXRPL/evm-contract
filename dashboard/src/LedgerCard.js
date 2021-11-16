import './LedgerCard.scss';

const LedgerCard = (props) => {

    const { nodeName, nodeHost, ledger } = props;
    return (
        <div className={"ledger-card border border-dark rounded shadow"}
            data-host={nodeHost}
            onClick={() => props.onClick && props.onClick()}>
            <div className="ledger-header clearfix">
                {nodeName && <div><strong className="pr-2 float-left">{nodeName}</strong></div>}
                {ledger && <div className="float-right">Ledger {ledger.seqNo}</div>}
            </div>
            <ul className="list-group list-group-flush ledger-content">
                <li className="list-group-item ledger-item text-truncate">
                    <i className="fas blue-icon fa-hashtag" title="Ledger hash"></i>
                    <span>{ledger ? ledger.hash : "-"}</span>
                </li>
                <li className="list-group-item ledger-item">
                    <i className="fas blue-icon fa-download" title="Inptus"></i>
                    {ledger && ledger.inputs && ledger.inputs.map((str, idx) => <span key={idx} className="badge badge-green mr-1">{str}</span>)}
                </li>
                <li className="list-group-item ledger-item">
                    <i className="fas blue-icon fa-upload" title="Outputs"></i>
                    {ledger && ledger.outputs && ledger.outputs.map((str, idx) => <span key={idx} className="badge badge-orange mr-1">{str}</span>)}
                </li>
            </ul>
        </div >
    )
}


export default LedgerCard;