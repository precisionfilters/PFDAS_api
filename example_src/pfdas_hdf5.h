/* -------------------------------------------------------------------------- */
/* (C) Copyright 2024 Precision Filters Inc., Ithaca, NY.                     */
/* All Rights Reserved.                                                       */
/* --------------------------------------------------------------------------
 * -------------------------------------------------------------------------- */
#ifndef __PFDAS_HDF5_H__
#define __PFDAS_HDF5_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>
void pfds_hdf5_write_attr_sr(double sr);

void pfdas_hdf5_set_channel_name(int ch, const char* name);
void pfds_hdf5_write_channel_names_attr();

void pfdas_hdf5_set_channel_mu_name(int ch, const char* name);
void pfdas_hdf5_set_channel_mu_scalar(int ch, double value);
void pfds_hdf5_write_channel_mu_attr();

void pfds_hdf5_nonblocking_write_packet(int threaded);
/**
* @brief Opens an HDF5 file for writing and initializes the dataset.
* @param fname The name of the HDF5 file to create.
* @param num_chans The number of channels in the dataset.
* @param dataset_size The size of the dataset.
* @return 0 on success, 1 on error.
 */
int pfdas_hdf5_open(const char* fname, int64_t num_chans, int64_t dataset_size);
/**
* @brief Checks if the HDF5 dataset is full.
* @return 1 if the dataset is full, 0 otherwise.
*/
int pfdas_hdf5_full();
/**
* @brief Closes the HDF5 file and releases resources.
*/
void pfdas_hdf5_close();
/**
* @brief Writes a packet of data to the HDF5 file.
* @param p A pointer to a PFDAS_base_packet_t structure containing the data to write.
*/
void pfdas_hdf5_write_packet(const struct PFDAS_base_packet_t* p);

void pfdas_hdf5_write_channel_names();
#ifdef __cplusplus
}
#endif
//
#endif
