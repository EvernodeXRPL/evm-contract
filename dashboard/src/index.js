import React from 'react';
import ReactDOM from 'react-dom';
import './index.scss';
import App from './App';

const clusterParam = "evm";
const clusterConfig = window.dashboardConfig.supportedClusters[clusterParam];
window.dashboardConfig.clusterKey = clusterParam;
window.dashboardConfig.clusterName = clusterConfig ? clusterConfig.clusterName : clusterParam;
window.dashboardConfig.submitInputs = clusterConfig ? clusterConfig.submitInputs : true;
window.dashboardConfig.clusterFound = true;

window.document.title = window.dashboardConfig.clusterName + " Cluster Dashboard";

if (window.isUnsupportedBrowser) {
    document.body.innerHTML =
        "<p class='lead m-4 text-center' style='font-size:2rem'>You are using an unsupported browser.<br />Sorry for the inconvenience.</p>";
}
else {

    window.adjustMapViewSize = function () {
        const mapView = document.getElementsByClassName("map-view")[0];
        if (mapView) {
            const w = 255;
            const h = 150;

            const parent = mapView.parentElement.parentElement;
            const maxw = parent.clientWidth;
            const maxh = parent.clientHeight;
            const ratio = Math.min(maxw / w, maxh / h);

            mapView.style.width = (w * ratio) + "px";
            mapView.style.height = (h * ratio) + "px";
        }
    }

    window.adjustLedgerScrollViewSize = function () {
        const ledgerScroll = document.getElementsByClassName("ledger-scroll-list")[0];
        if (ledgerScroll) {
            const nodeNumBar = document.getElementsByClassName("node-num-bar")[0];
            const inputForm = document.getElementsByClassName("input-form")[0];
            const formHeight = inputForm ? inputForm.clientHeight : 0;
            const numBarHeight = nodeNumBar ? nodeNumBar.clientHeight : 0;
            ledgerScroll.style.height = (document.body.clientHeight - formHeight - numBarHeight - 120) + "px"
        }
    }

    window.onresize = () => {
        window.adjustMapViewSize();
        window.adjustLedgerScrollViewSize();
    };

    ReactDOM.render(
        <App />,
        document.getElementById('root'),
        window.adjustMapViewSize
    );
}