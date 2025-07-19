import Graph from "./Graph";

function Content({ selectedSymbol }: { selectedSymbol: string }) {
    return (
        <div className="w-full m-4 flex flex-col gap-4">
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["close", "sma", "ema_short", "ema_long"]}
            />
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["signal", "macd"]}
            />
            <Graph selectedSymbol={selectedSymbol} indicators={["distance"]} />
        </div>
    );
}

export default Content;
