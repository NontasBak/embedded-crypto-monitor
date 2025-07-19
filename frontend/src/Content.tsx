import Graph from "./Graph";

function Content({ selectedSymbol }: { selectedSymbol: string }) {
    return (
        <div className="w-full m-4">
            <Graph
                selectedSymbol={selectedSymbol}
                indicators={["ema_short", "ema_long", "close"]}
            />
        </div>
    );
}

export default Content;
