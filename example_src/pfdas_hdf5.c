#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "include/hdf5.h"
#include "PFDAS_api.h"
#include "PFDAS_fpga_payload.h"
#include "pfdas_hdf5.h"
//
#define NCHAN 16         // number of channels
#define CHUNK_SIZE 8192
#define MAX_CHAN_NAME_LEN 20  // maximum length of a channel name
//
static hid_t memspace_id;
static hid_t dset_id;
static hid_t dcpl_id;
static hid_t space_id;
static hid_t file_id;
static hid_t attr_id_sr;
static hid_t attr_id_sr_measured;
static hid_t attr_id_sr_changed;
static hsize_t offset[2] = {0, 0};
static hsize_t count[2] = {NCHAN, CHUNK_SIZE};
static float* data = 0;
static int hdf5_opened = 0;
static int hdf5_closed = 0;
static int threaded_write = 0;
static int thread_running = 0;
//
void _hdf5_write();
void _hdf5_write_entry();
//

static hid_t dset_id_channel_names = 0;
static hid_t space_id_channel_names = 0;
static hid_t str_type_channel_names = 0;
int64_t last_t0 = 0;
int64_t last_dt = 0;
int64_t dt_changed = 0;
double sr = 0;

// void _create_chan_names() {
//     // Create a dataspace for the channel names
//     hsize_t dims[1] = {NCHAN}; // 1-dimensional dataspace with NCHAN elements
//     space_id_channel_names = H5Screate_simple(1, dims, NULL); // H5Screate_simple creates a simple dataspace with the specified dimensions
//     str_type_channel_names = H5Tcopy(H5T_C_S1);               // Create a datatype for the channel names
//     herr_t status = H5Tset_size(str_type_channel_names, MAX_CHAN_NAME_LEN); // H5Tcopy creates a copy of the specified datatype (in this case, a C-style string)
//     // H5Tset_size sets the size of the datatype (in this case, the maximum length of a channel name)
//     dset_id_channel_names = H5Dcreate(file_id, "channel_names", str_type_channel_names, space_id_channel_names, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); // Create a dataset for the channel names
//     // H5Dcreate creates a dataset with the specified name, datatype, and dataspace
//     //
// }
// void pfdas_hdf5_write_channel_names(){
//     // char *chan_names_block = (char *)malloc(NCHAN * MAX_CHAN_NAME_LEN * sizeof(char)); // Create a contiguous block of memory to hold all the strings
//     // for (int i = 0; i < NCHAN; i++) { // Fill the block of memory with the channel names
//     //     sprintf(chan_names_block + i * MAX_CHAN_NAME_LEN, "Channel %d", i + 1);
//     // }
//     // herr_t status = H5Dwrite(dset_id_channel_names, str_type_channel_names, space_id_channel_names, space_id_channel_names, H5P_DEFAULT, chan_names_block); // Write the channel names to the dataset
//     // free(chan_names_block); // Close the HDF5 resources
//     //
//     H5Dwrite(dset_id_channel_names, str_type_channel_names, space_id_channel_names, space_id_channel_names, H5P_DEFAULT, channel_names_buf); // Write the channel names to the dataset
// }

char channel_names_buf[NCHAN*MAX_CHAN_NAME_LEN] = {0};
void pfdas_hdf5_set_channel_name(int ch, const char* name){
    if(ch < NCHAN){
        sprintf(channel_names_buf + ch * MAX_CHAN_NAME_LEN, "%s", name);
    }
}
// hsize_t attr_dims[1] = {1}; // scalar dataspace
// hid_t attr_space_id = H5Screate_simple(1, attr_dims, NULL);
// if (attr_space_id < 0) {
//     fprintf(stderr, "Error creating attribute dataspace\n");
//     return 1;
// }

// attr_id_sr = H5Acreate(dset_id, "sample_rate", H5T_IEEE_F64LE, attr_space_id, H5P_DEFAULT, H5P_DEFAULT);

void pfds_hdf5_write_channel_names_attr(){
    hsize_t dims[1] = {NCHAN}; // 1-dimensional dataspace with NCHAN elements
    hid_t attr_space = H5Screate_simple(1, dims, NULL);
    // Create a dataspace for the channel names
    hid_t str_type = H5Tcopy(H5T_C_S1);               // Create a datatype for the channel names
    H5Tset_size(str_type, MAX_CHAN_NAME_LEN);  // set size to variable
    hid_t attr_id = H5Acreate(dset_id, "channel_names", str_type, attr_space, H5P_DEFAULT, H5P_DEFAULT);

    // // Assume you have an array of null-terminated strings to write
    // char *chan_names_block = (char *)malloc(NCHAN * MAX_CHAN_NAME_LEN * sizeof(char)); // Create a contiguous block of memory to hold all the strings
    // memset(chan_names_block, 0, NCHAN*MAX_CHAN_NAME_LEN);
    // for (int i = 0; i < NCHAN; i++) { // Fill the block of memory with the channel names
    //     sprintf(chan_names_block + i * MAX_CHAN_NAME_LEN, "Channel %d", i + 1);
    // }
    // Write the data to the attribute
    H5Awrite(attr_id, str_type, channel_names_buf);
    // Free the buffer
   // free(chan_names_block);

}
char channel_mu_buf[NCHAN*MAX_CHAN_NAME_LEN] = {0};
double channel_mu_scalar[NCHAN] = {1.0};
void pfdas_hdf5_set_channel_mu_name(int ch, const char* name){
    if(ch < NCHAN){
        sprintf(channel_mu_buf + ch * MAX_CHAN_NAME_LEN, "%s", name);
    }
}
void pfdas_hdf5_set_channel_mu_scalar(int ch, double value){
    if(ch < NCHAN){
        channel_mu_scalar[ch] = value;
    }
}
// hsize_t attr_dims[1] = {1}; // scalar dataspace
// hid_t attr_space_id = H5Screate_simple(1, attr_dims, NULL);
// if (attr_space_id < 0) {
//     fprintf(stderr, "Error creating attribute dataspace\n");
//     return 1;
// }

// attr_id_sr = H5Acreate(dset_id, "sample_rate", H5T_IEEE_F64LE, attr_space_id, H5P_DEFAULT, H5P_DEFAULT);

void pfds_hdf5_write_channel_mu_attr(){
    hsize_t dims[1] = {NCHAN}; // 1-dimensional dataspace with NCHAN elements
    hid_t attr_space = H5Screate_simple(1, dims, NULL);
    // Create a dataspace for the channel names
    hid_t str_type = H5Tcopy(H5T_C_S1);               // Create a datatype for the channel names
    H5Tset_size(str_type, MAX_CHAN_NAME_LEN);  // set size to variable
    hid_t attr_id = H5Acreate(dset_id, "measurement_units", str_type, attr_space, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attr_id, str_type, channel_mu_buf);
}
//
int pfdas_hdf5_open(const char* fname, int64_t num_chans, int64_t dataset_size){
    int64_t chunks = dataset_size/CHUNK_SIZE;
    printf("chunks=%lld\n", chunks);
    chunks -= 1;
    dataset_size = chunks*CHUNK_SIZE; // Force the dataset_size to be a even multiple of the chunk_size.
    printf("dataset_size=%lld\n", dataset_size);
    if(threaded_write){
        PFDAS_thread_run(_hdf5_write_entry);
    }
    file_id = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Error creating file\n");
        return 0;
    }
    //
    //
    hsize_t dims[2] = {num_chans, dataset_size}; // Create the dataset space
    if((space_id = H5Screate_simple(2, dims, NULL)) < 0){
        fprintf(stderr, "Error creating dataspace\n");
        return 0;
    }
    // Create the dataset creation property list
    dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
    if (dcpl_id < 0) {
        fprintf(stderr, "Error creating property list\n");
        return 0;
    }
    // Set the chunk size
    hsize_t chunk_dims[2] = {NCHAN, CHUNK_SIZE};
    H5Pset_chunk(dcpl_id, 2, chunk_dims);
    if (H5Pset_chunk(dcpl_id, 2, chunk_dims) < 0) {
        fprintf(stderr, "Error setting chunk size\n");
        return 0;
    }
    // Create the dataset
    if ((dset_id = H5Dcreate(file_id, "my_dataset", H5T_NATIVE_FLOAT, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
        fprintf(stderr, "Error creating dataset\n");
        return 0;
    }

    if ((memspace_id = H5Screate_simple(2, count, NULL)) < 0) {
        fprintf(stderr, "Error creating memory space\n");
        return 0;
    }
    data = (float *)malloc(CHUNK_SIZE * NCHAN * sizeof(float));
    if (data == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return 0;
    }
    //
    hsize_t attr_dims[1] = {1}; // scalar dataspace
    hid_t attr_space_id = H5Screate_simple(1, attr_dims, NULL);
    if (attr_space_id < 0) {
        fprintf(stderr, "Error creating attribute dataspace\n");
        return 0;
    }

    attr_id_sr = H5Acreate(dset_id, "sample_rate", H5T_IEEE_F64LE, attr_space_id, H5P_DEFAULT, H5P_DEFAULT);
    attr_id_sr_measured = H5Acreate(dset_id, "sample_rate_measured", H5T_IEEE_F64LE, attr_space_id, H5P_DEFAULT, H5P_DEFAULT);
    attr_id_sr_changed = H5Acreate(dset_id, "sample_rate_changed_count", H5T_IEEE_F64LE, attr_space_id, H5P_DEFAULT, H5P_DEFAULT);
    //
    hdf5_opened = 1;
    hdf5_closed = 0;
    return PFDAS_data_reblock_init(CHUNK_SIZE, num_chans);
}
void _pfds_hdf5_write_attr_sr(double sr_meas, double sr_chg_cnt){
    H5Awrite(attr_id_sr_measured, H5T_IEEE_F64LE, &sr_meas);
    H5Awrite(attr_id_sr_changed, H5T_IEEE_F64LE, &sr_chg_cnt);
}
void pfds_hdf5_write_attr_sr(double sr){
    H5Awrite(attr_id_sr, H5T_IEEE_F64LE, &sr);
}

void pfdas_hdf5_close(){
    if(hdf5_closed == 0){
        _pfds_hdf5_write_attr_sr(sr, dt_changed);
        printf("Closing h5 file.");
        H5Fclose(file_id); // This closes all objects that have been opened.
        hdf5_closed = 1;
        free(data);
    }
}
//
static uint32_t sec = 0;             // Temporary storage for seconds.
static uint32_t nsec = 0;            // Temporary storage for nano seconds.
static uint64_t written = 0;
static uint64_t max_size = 0;
int pfdas_hdf5_full(){
    if(hdf5_closed){ return 1; }
    //Check if the dataset is full
    if(max_size == 0){
        hsize_t maxsize[2];
        H5Sget_simple_extent_dims(space_id, maxsize, NULL);
        max_size = maxsize[1];
    }
    if (written >= max_size) {
        printf("Dataset is full, done writing data. max=%llu current=%llu\n", max_size, written);
        return 1;
    }
   // printf("Dataset : max=%llu current=%llu\n", max_size, written);
    return 0;
}
void _hdf5_write_entry(){
    thread_running = 1;
    while(1){
        PFDAS_semaphore_wait();
        PFDAS_mutex_lock();
        _hdf5_write();
        PFDAS_mutex_unlock();
    }
    thread_running = 0;
}
int print_status = 10;
double last_percent = 0;
void _hdf5_write(){
    written += CHUNK_SIZE;
    // Select the hyperslab in the dataset
    herr_t status = H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, NULL, count, NULL);
    if (status < 0) { fprintf(stderr, "Error selecting hyperslab\n"); }
    // Write the chunk
    status = H5Dwrite(dset_id, H5T_NATIVE_FLOAT, memspace_id, space_id, H5P_DEFAULT, data);
    if (status < 0) { fprintf(stderr, "Error writing chunk\n"); }
    // Update the offset
    offset[1] += CHUNK_SIZE;
    if(print_status-- == 0){
        double sofar = written;
        double total = max_size;
        double percent = sofar/total*100.0;
        if(percent - last_percent > 1.0){
            printf("%s Written %.2f%%.\n", __FUNCTION__, percent);
            last_percent = percent;
        }
        fflush(stdout);
        print_status = 10;
        //printf("%s Writting %llu of %llu.\n", __FUNCTION__, written, max_size);
    }
}
void pfds_hdf5_nonblocking_write_packet(int threaded){
    threaded_write = true;
}
void pfdas_hdf5_write_packet(const struct PFDAS_base_packet_t* p){
    //
    uint32_t sec = 0;
    uint32_t nsec = 0;
    int32_t pack = -1;
    int32_t samples = -1;
    int error = PFDAS_fpga_ch_data_sample_info((uint8_t*)(p+1), p->payload_size, &pack, &samples);
    if(error < PFDAS_API_NO_ERROR){
        if(error == PFDAS_API_ERROR1_BUFFER_SIZE_LT_PAYLOAD_SAMPLES){
            //printf("ch%d is muted\n", p->channel);
            // This case is for a channel that has been muted, we get the packet but the payload size does not have the data.
            // Our packet memory does have the space allocated beyond the packket size which we can use without fear.
            // First we need to get figure out what the payload size should be.
            int32_t sample_bytes = p->payload_size;
            if(pack == 0) { sample_bytes += samples * 4; }
            if(pack == 1) { sample_bytes += samples * 3; }
            if(pack == 2) { sample_bytes += samples * 2; }
            int error = PFDAS_fpga_ch_data_sample_info((uint8_t*)(p+1), sample_bytes, &pack, &samples);
            if(error < PFDAS_API_NO_ERROR){
                printf("we have an payload size error BIG PROBLEM (%s)!!!\n", PFDAS_error1_to_string(error));
            }else{
                ((struct PFDAS_base_packet_t*)p)->payload_size = sample_bytes;
                int32_t zeros[65536] = {0};
                PFDAS_fpga_ch_data_overwrite(zeros, samples, (uint8_t*)(p+1), sample_bytes);
            }
        }else{
            printf("we have an payload size error (%s)!!!\n", PFDAS_error1_to_string(error));
            exit(0);
        }
    }
    if(p->channel == 0){ // Use channel 0 as our time keeping information.
        PFDAS_fpga_ch_data_gettime((uint8_t*)(p+1), p->payload_size, &sec, &nsec);
        int64_t t0 = PFDAS_ptp_uint32_2_uint64_secs(sec, nsec);
        if(last_t0 != 0){
            int64_t dt = t0 - last_t0;
            last_dt = dt;
            if(last_dt != dt) { dt_changed++; }
            sr = (double)samples/(double)dt;
            sr *= 1000000000; // Convert from samples/nanosecond to sample/second.
           // printf("dt=%lld dt_changed=%lld sr=%f\n", dt, dt_changed, sr);
        }
        last_t0 = t0;
    }
    int rerror = 0;
    if((rerror = PFDAS_data_reblock_append(p)) < 0){
        printf("reblock append error=%d\n", rerror);
    }
    if(p->channel == NCHAN-1){
        int all_exceed = 1;
        while(all_exceed){
            for(int i=0; i<NCHAN; i++){
                if(PFDAS_data_reblock_length(i) < CHUNK_SIZE){ all_exceed = 0; }
            }
            if(all_exceed){
                //
                PFDAS_mutex_lock(); // We have to lock assess to data as it is shared between here and the write thread.
                float cnt2volts = PFDAS_cnt_to_volt_scalar(); // This is a constant used to scale counts to volts.
                for (int j = 0; j < NCHAN; j++) {
                    int32_t* data_p = PFDAS_data_reblock_data(j);
                    for (int k = 0; k < CHUNK_SIZE; k++) {
                        float sample = (float)data_p[k]/cnt2volts;
                        sample *= channel_mu_scalar[j];
                        data[j * CHUNK_SIZE + k] = sample;
                    }
                }
                //
                PFDAS_mutex_unlock();
                PFDAS_data_reblock_shift(CHUNK_SIZE);
                if(threaded_write){
                    PFDAS_semaphore_post(); // Calls _hdf5_write() which is a thread waiting to write out the data to disk.
                }else{
                    _hdf5_write();
                }
            }
        }
    }
}
