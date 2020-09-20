-- fields
local dt_text = ProtoField.string("debug_trace.text", "Text")
local dt_type = ProtoField.string("debug_trace.type", "Type")

-- declare dissector
local debug_trace_dissector = Proto.new("debug_trace", "Trace")

debug_trace_dissector.fields = {
    dt_text,
    dt_type,
}

ip_proto_f = Field.new("ip.proto")

function debug_trace_dissector.dissector(tvbuf,pktinfo,root)
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(debug_trace_dissector, tvbuf:range(0,pktlen))

    -- Set a trace type to show
    local ipProto = tostring(ip_proto_f())
    local type = ""
    if ipProto == "252" then
        type = "Error"
    elseif ipProto == "251" then
        type = "Warning"
    elseif ipProto == "250" then
        type = "Debug"
    else
        type = "Undefined"
    end
    tree:add(dt_type, type)
    pktinfo.cols.protocol:set("Trace: " .. type)

    -- Convert data into a string and show it in info column
    local s = ""
    local c
    for i=0, pktlen - 1, 1
    do
        c = string.char(tvbuf:range(i,1):uint())
        -- Ignore line break to prevent adding extra line and thus prevent expanding height of Wireshark line
        if c ~= "\n" then
            s = s .. c
        end
    end
    tree:add(dt_text, s)
    pktinfo.cols.info = s
end

--Asign to protocol 0xfa, 0xfb, 0xfc: debug_trace
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0xfa, debug_trace_dissector) -- debug
ipProtocol:add(0xfb, debug_trace_dissector) -- warning
ipProtocol:add(0xfc, debug_trace_dissector) -- error
