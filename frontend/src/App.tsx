import AppSidebar from "./Sidebar";
import Content from "./Content";
import { ThemeProvider } from "@/components/theme-provider";
import { ModeToggle } from "./components/mode-toggle";
import { useState } from "react";
import { SYMBOLS } from "@/lib/symbols";

function App() {
    const [selectedSymbol, setSelectedSymbol] = useState<string>(
        SYMBOLS[0].name,
    );

    return (
        <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
            <main className="flex gap-2">
                <AppSidebar
                    selectedSymbol={selectedSymbol}
                    setSelectedSymbol={setSelectedSymbol}
                />
                <Content selectedSymbol={selectedSymbol} />
                <ModeToggle />
            </main>
        </ThemeProvider>
    );
}

export default App;
