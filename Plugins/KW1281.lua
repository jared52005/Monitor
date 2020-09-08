-- KW1281 description: https://www.blafusel.de/obd/obd2_kw1281.html
-- constants
local kw_etx = 0x03 --ETX byte which is on the end of KW1281 packet

local kw_block_title = {
    [0x05] = "Clear All Errors",
    [0x06] = "End communication",
    [0x07] = "Read All Errors",
    [0x09] = "ACK",
    [0x29] = "Group reading",
    [0xE7] = "Antwort auf group reading Aufforderung",
    [0xF6] = "ASCII",
    [0xFC] = "Antwort auf get errors Aufforderung",
}

-- fields
local kw_length = ProtoField.uint8("kw1281.length", "Payload Length", base.DEC)
local kw_cnt = ProtoField.uint8("kw1281.cnt", "Block counter", base.HEX)
local kw_block = ProtoField.uint8("kw1281.blocktype", "Block Title", base.HEX, kw_block_title)
local kw_data = ProtoField.new("Data", "kw1281.data", ftypes.BYTES) --Data for next layer
local kw_cs_valid = ProtoField.string("kw1281.status", "Block status") --Correct length and correct ETX byte

-- declare dissector
local kw1281_dissector = Proto.new("kw1281", "KW1281")

kw1281_dissector.fields = {
    kw_length,
    kw_cnt,
    kw_block,
    kw_data,
    kw_cs_valid,
}

function kw1281_dissector.dissector(tvbuf,pktinfo,root)
    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("KW1281")
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(kw1281_dissector, tvbuf:range(0,pktlen))

    -- Parse LEN byte
    local frame_len = tvbuf:range(0,1):uint()
    local data_len = frame_len - 3
    tree:add(kw_length, tvbuf:range(0,1))
    -- Parse Block Counter
    tree:add(kw_cnt, tvbuf:range(1,1))
    -- Parse Block Title
    local block = tvbuf:range(2,1):uint()
    tree:add(kw_block, tvbuf:range(2,1))
    -- Parse ETX (Block End)
    local etx = tvbuf:range(pktlen - 1, 1):uint()
    if(etx == kw_etx) then
        tree:add(kw_cs_valid, "Good")
    else
        tree:add(kw_cs_valid, "Bad")
    end
    
    -- Show data, if there are any
    if (data_len > 0) then
        tree:add(kw_data, tvbuf:range(3,data_len))
    end

    --Show packet information in info column
    if kw_block_title[block] ~= nil then
        pktinfo.cols.info = kw_block_title[block]
    end
end

--Asign to protocol 0x92: KW1281
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0x92, kw1281_dissector)
