import { LineChart } from "@mui/x-charts";
import { useEffect, useState } from "react";
import axios from "axios";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";

const SYMBOLS: string[] = [
    "BTC-USDT",
    "ADA-USDT",
    "ETH-USDT",
    "DOGE-USDT",
    "XRP-USDT",
    "SOL-USDT",
    "LTC-USDT",
    "BNB-USDT",
];

type CryptoData = {
    [key: string]: {
        values: number[];
        timestamps: number[];
    };
};

function initializeData(): CryptoData {
    const initialData: CryptoData = {};
    SYMBOLS.forEach((symbol) => {
        initialData[symbol] = {
            values: [],
            timestamps: [],
        };
    });
    return initialData;
}

function Graph() {
    const [SMAData, setSMAData] = useState<CryptoData>(initializeData());
    const [selectedSymbol, setSelectedSymbol] = useState<string>(SYMBOLS[0]);

    async function fetchData(symbol: string): Promise<{ data: { values: number[]; timestamps: number[] } }> {
        return await axios.get(`http://localhost:8080/sma?symbol=${symbol}&window=100`);
    }

    const dateFormatter = Intl.DateTimeFormat(undefined, {
        hour: "2-digit",
        minute: "2-digit",
    });

    useEffect(() => {
        const promises = SYMBOLS.map(async (symbol) => {
            const symbolData = await fetchData(symbol);
            console.log(symbolData);
            setSMAData((prevData) => ({
                ...prevData,
                [symbol]: {
                    values: symbolData.data.values,
                    timestamps: symbolData.data.timestamps,
                },
            }));
        });

        Promise.all(promises).then(() => {
            console.log("Data fetched");
        });
    }, []);

    return (
        <div style={{ width: "50%" }}>
            <LineChart
                height={300}
                skipAnimation
                series={[{ data: SMAData[selectedSymbol].values, showMark: false }]}
                xAxis={[
                    {
                        scaleType: "point",
                        data: SMAData[selectedSymbol].timestamps,
                        valueFormatter: (value: Date) => dateFormatter.format(value),
                    },
                ]}
                yAxis={[{ width: 50 }]}
                margin={{ right: 24 }}
            />
            {/* {JSON.stringify(SMAData["BTC-USDT"])} */}
            <div className="w-1/2">
                <Select value={selectedSymbol} onValueChange={setSelectedSymbol}>
                    <SelectTrigger>
                        <SelectValue placeholder="Select an option" />
                    </SelectTrigger>
                    <SelectContent>
                        {SYMBOLS.map((symbol) => {
                            return <SelectItem value={symbol}>{symbol}</SelectItem>;
                        })}
                    </SelectContent>
                </Select>
            </div>
        </div>
    );
}

export default Graph;
