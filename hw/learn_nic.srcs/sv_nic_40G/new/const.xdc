create_clock -period 10.000 -name sys_clk [get_ports pcie_clk_p]
set_property PACKAGE_PIN AR15 [get_ports pcie_clk_p]
set_property IOSTANDARD LVCMOS18 [get_ports negative_process_indicator]
set_property IOSTANDARD LVCMOS18 [get_ports phy_rdy_out]
set_property IOSTANDARD LVCMOS18 [get_ports positive_process_indicator]
set_property IOSTANDARD LVCMOS18 [get_ports user_lnk_up]
set_property PACKAGE_PIN BH24 [get_ports phy_rdy_out]
set_property PACKAGE_PIN BG24 [get_ports user_lnk_up]
set_property PACKAGE_PIN BG25 [get_ports negative_process_indicator]
set_property PACKAGE_PIN BF25 [get_ports positive_process_indicator]



set_false_path -from [get_pins user_core_inst/positive_process_inst/eth_40G_top_inst/eth_40G_wrapper_inst/eth_40G_i/hier_0/l_ethernet_0/inst/i_eth_40G_l_ethernet_0_0_top_0/i_HSEC_CORES/i_RX_TOP/i_RX_CORE/i_RX_DESTRIPER/i_RX_LANE_ALIGNER/aligned_reg/C] -to [get_pins user_core_inst/positive_process_inst/eth_link_connected_s_meta1_reg/D]
set_false_path -from [get_pins user_core_inst/positive_process_inst/eth_40G_top_inst/eth_40G_wrapper_inst/eth_40G_i/hier_0/l_ethernet_0/inst/i_eth_40G_l_ethernet_0_0_top_0/i_HSEC_CORES/i_RX_TOP/i_RX_CORE/i_RX_DESTRIPER/i_RX_LANE_ALIGNER/hi_ber_reg/C] -to [get_pins user_core_inst/positive_process_inst/eth_link_connected_s_meta1_reg/D]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/wr_ptr_gray_r_reg[0]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/wr_ptr_gray_scdc_r_reg[0]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/wr_ptr_gray_r_reg[1]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/wr_ptr_gray_scdc_r_reg[1]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/rd_ptr_gray_r_reg[0]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/rd_ptr_gray_scdc_r_reg[0]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/rd_ptr_gray_r_reg[1]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/rd_ptr_gray_scdc_r_reg[1]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/rd_ptr_gray_r_reg[1]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/rd_ptr_gray_scdc_r_reg[1]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/rd_ptr_gray_r_reg[0]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/rx_async_fifo_huge_inst/rd_ptr_gray_scdc_r_reg[0]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/wr_ptr_gray_r_reg[1]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/wr_ptr_gray_scdc_r_reg[1]/D}]
set_false_path -from [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/wr_ptr_gray_r_reg[0]/C}] -to [get_pins {user_core_inst/positive_process_inst/eth_40G_top_inst/tx_async_fifo_huge_inst/wr_ptr_gray_scdc_r_reg[0]/D}]

set_false_path -from [get_pins user_core_inst/positive_process_inst/eth_40G_top_inst/eth_40G_wrapper_inst/eth_40G_i/hier_0/l_ethernet_0/inst/i_eth_40G_l_ethernet_0_0_top_0/i_HSEC_CORES/i_RX_TOP/i_RX_CORE/i_RX_DESTRIPER/i_RX_LANE_ALIGNER/aligned_reg_replica/C] -to [get_pins user_core_inst/positive_process_inst/eth_link_connected_s_meta1_reg/D]
set_false_path -to [get_pins user_core_inst/positive_process_inst/eth_link_connected_s_meta1_reg/D]

