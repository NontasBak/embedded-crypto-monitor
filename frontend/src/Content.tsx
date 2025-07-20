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
            />
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["signal", "macd"]}
                selectedWindow={selectedWindow}
            />
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["distance"]}
                selectedWindow={selectedWindow}
            />
        </div>
    );
}

export default Content;
