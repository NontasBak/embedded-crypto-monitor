import { TokenIcon } from "@web3icons/react";
import {
    Sidebar,
    SidebarContent,
    SidebarGroup,
    SidebarGroupContent,
    SidebarGroupLabel,
    SidebarMenu,
    SidebarMenuButton,
    SidebarMenuItem,
    SidebarProvider,
} from "@/components/ui/sidebar";
import { Coins } from "lucide-react";
import { SYMBOLS } from "@/lib/symbols";
import { ModeToggle } from "./components/mode-toggle";

function AppSidebar({
    selectedSymbol,
    setSelectedSymbol,
}: {
    selectedSymbol: string;
    setSelectedSymbol: (symbol: string) => void;
}) {
    return (
        <div className="w-min h-screen">
            <SidebarProvider>
                <Sidebar>
                    <SidebarContent>
                        <SidebarGroup className="flex flex-col gap-4 flex-1">
                            <SidebarGroupLabel className="text-xl py-6 flex gap-4 tracking-wider">
                                <Coins />
                                Crypto Monitor
                            </SidebarGroupLabel>
                            <SidebarGroupContent className="flex-1">
                                <SidebarMenu>
                                    {SYMBOLS.map((symbol) => (
                                        <SidebarMenuItem key={symbol.title}>
                                            <SidebarMenuButton
                                                asChild
                                                size="lg"
                                                className="text-lg"
                                                isActive={
                                                    selectedSymbol ===
                                                    symbol.name
                                                }
                                            >
                                                <a
                                                    href={symbol.url}
                                                    onClick={() =>
                                                        setSelectedSymbol(
                                                            symbol.name,
                                                        )
                                                    }
                                                >
                                                    <TokenIcon
                                                        symbol={symbol.icon}
                                                        variant="mono"
                                                        size="64"
                                                        className="icon"
                                                    />
                                                    <span>
                                                        {symbol.title.toUpperCase()}
                                                    </span>
                                                </a>
                                            </SidebarMenuButton>
                                        </SidebarMenuItem>
                                    ))}
                                </SidebarMenu>
                            </SidebarGroupContent>
                        </SidebarGroup>
                        <div className="p-4 mt-auto">
                            <ModeToggle />
                        </div>
                    </SidebarContent>
                </Sidebar>
            </SidebarProvider>
        </div>
    );
}

export default AppSidebar;
