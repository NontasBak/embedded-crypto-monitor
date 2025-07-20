import Graph from "./Graph";
import WindowSelector from "./WindowSelector";
import { useState } from "react";

function Content({ selectedSymbol }: { selectedSymbol: string }) {
    const [selectedWindow, setSelectedWindow] = useState<string>("360"); // Default to 6 hours

    return (
        <div className="w-full m-4 flex flex-col gap-4">
            <WindowSelector
                selectedWindow={selectedWindow}
                onWindowChange={setSelectedWindow}
            />
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["close", "sma", "ema_short", "ema_long"]}
                selectedWindow={selectedWindow}
                title="Price and Moving Averages"
                description="Closing price with simple moving average (SMA) and exponential moving averages (EMA)"
            />
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["signal", "macd"]}
                selectedWindow={selectedWindow}
                title="MACD and Signal Line Indicator"
                description="Moving Average Convergence Divergence (MACD) with Signal line for trend analysis"
            />
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["distance"]}
                selectedWindow={selectedWindow}
                title="Distance Metric"
                description="Distance measurement indicator for technical analysis"
            />
        </div>
    );
}

export default Content;
