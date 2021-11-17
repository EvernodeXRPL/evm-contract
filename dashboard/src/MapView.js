import React from "react"
import HP from "./HPService"
import MapNode from "./MapNode"
import './MapView.scss';

class MapView extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            regionList: []
        }
    }

    componentDidMount() {

        const onUpdated = (regionList) => {
            this.setState({
                regionList: regionList
            })
        };

        HP.nodeManager.on(HP.events.regionListUpdated, onUpdated);
    }

    render() {
        const nodeList = Object.values(this.state.regionList);
        return (
            <div className="content flex-fill d-flex align-items-center">
                <div className="map-view-container">
                    <div className="map-view">
                        {nodeList.map((n, idx) => <MapNode key={idx} region={n} />)}
                    </div>
                </div>
            </div>)
    }
}

export default MapView;
