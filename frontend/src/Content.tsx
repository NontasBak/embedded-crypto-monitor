import Graph from "./Graph";

function Content({ selectedSymbol }: { selectedSymbol: string }) {
    return (
        <div className="w-1/2 m-4">
            <Graph selectedSymbol={selectedSymbol} />
        </div>
    );
}

export default Content;
