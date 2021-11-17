import React from "react"
import HP from "./HPService"
import LedgerCard from "./LedgerCard"
import { CSSTransitionGroup } from 'react-transition-group'
import './LedgerCardList.scss';

class LedgerCardList extends React.Component {

    constructor(props) {
        super(props);
        this.state = {}
    }

    componentDidMount() {
        HP.nodeManager.on(HP.events.ledgerUpdated, (updatedNode) => {
            if (updatedNode === this.props.node)
                this.setState(this.state)
        });

        window.adjustLedgerScrollViewSize();
    }

    render() {
        const { node } = this.props;
        const ledgers = node && node.ledgers.map((ledger, idx) =>
            <div key={ledger.seqNo}>
                <div className="m-1">
                    <LedgerCard ledger={ledger} />
                </div>
            </div>
        );
        return (
            <div className="ledger-scroll-list d-none d-lg-flex flex-column p-1">
                <div className="flex-fill">
                    {/* Filler space to push the card list down */}
                </div>
                <div className="card-list-container">
                    <div className="card-list pb-1">
                        {ledgers &&
                            <CSSTransitionGroup
                                transitionName="ledger"
                                transitionEnterTimeout={500}
                                transitionLeaveTimeout={500}>
                                {ledgers}
                            </CSSTransitionGroup>}
                    </div>
                </div>
            </div>
        )
    }
}

export default LedgerCardList;