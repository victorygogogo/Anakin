#include "test_lite.h"
#include "saber/lite/utils/cv_utils.h"
#include "cv_utils_basic.h"
#include <random>

using namespace anakin::saber;
using namespace anakin::saber::lite;

int g_cluster = 0;
int g_threads = 1;
int g_h = 1920;
int g_w = 1080;
int g_ww = 960;
int g_hh = 540;
int g_angle = 180;
int g_flip_num = 1;
int g_compared = 0;
std::string g_img_name = "test.pgm";
std::string g_file_path = "/data/local/tmp/lite/";
typedef Tensor<CPU> TensorHf4;
typedef Tensor<CPU> TensorH4;

#define COMPARE_RESULT 1

void fill_tensor_host_rand(char* dio, long long size) {
    for (long long i = 0; i < size; ++i) {
        dio[i] = rand() % 256 ;//-128;
    }
}

template <typename Tensor_t>
void print_tensor_int8(const Tensor_t& tensor) {
    printf("host tensor data size: %d\n", tensor.size());
    const unsigned char* data_ptr = (const unsigned char*)tensor.get_buf()->get_data();
    int size = tensor.size();
    for (int i = 0; i < size; ++i) {
        printf("%3d ", data_ptr[i]);
        if ((i + 1) % tensor.width() == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

template <typename Dtype>
void tensor_cmp_host_int8(const Dtype* src1, const Dtype* src2, \
                     int size, double& max_ratio, double& max_diff) {

    const double eps = 1e-6f;
    max_diff = fabs(src1[0] - src2[0]);
    max_ratio = 2.0 * max_diff / (src1[0] + src2[0] + eps);

    for (int i = 1; i < size; ++i) {
        double diff = fabs(src1[i] - src2[i]);

        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (src1[i] + src2[i] + eps);
        }
    }
}

#if 1
TEST(TestSaberLite, test_func_cv_bgr_resize) {
    LOG(INFO) << "test_func_cv_bgr_resize start";
    // start Reshape & doInfer
    Context ctx1;
    LOG(INFO) << "set runtine context";
    PowerMode mode = g_cluster == 0? SABER_POWER_HIGH : SABER_POWER_LOW;
    ctx1.set_run_mode(mode, g_threads);
    LOG(INFO) << "test threads activated";
#pragma omp parallel
    {
#ifdef USE_OPENMP
        int thread = omp_get_num_threads();
        LOG(INFO) << "number of threads: " << thread;
#endif
    }

    int test_iter = 1;

    int w_in = g_w;
    int h_in = g_h;
    int w_out = g_ww;
    int h_out = g_hh;

    LOG(INFO) << " input tensor size, num=" << 1 << ", channel=" << \
        1 << ", height=" << h_in << ", width=" << w_in;
    LOG(INFO) << " output tensor size, num=" << 1 << ", channel=" << \
        1 << ", height=" << h_out << ", width=" << w_out;

    //Tensor<CPU, AK_UINT8> thin(shape_in);
    int size = 3 * h_in * w_in / 2;
    unsigned char* nv21 = new unsigned char[size];

    std::string file_name = g_file_path + g_img_name;
    LOG(INFO) << "input image name " << file_name;

    LOG(INFO) << "read image";
    read_pgm(file_name.c_str(), nv21, h_in, w_in);
    // for (int i = 0; i < size; ++i) {
    //     nv21[i] = (unsigned char)i;
    // }
    // fill_tensor_host_rand(nv21, size);

    int out_size = 3 * h_out * w_out;
    unsigned char* bgr = new unsigned char[out_size];
    unsigned char* bgr_basic = new unsigned char[out_size];

    float width_scale = (float)w_in / w_out;
    float height_scale = (float)h_in / h_out;

    //bgr
    unsigned char* tmp_basic = new unsigned char[size * 2];
    unsigned char* tmp = new unsigned char[size * 2];

    unsigned char* tv_out_ratote = new unsigned char[size * 2];
    unsigned char* tv_out_ratote_basic = new unsigned char[size * 2];

    unsigned char* tv_out_flip = new unsigned char[size * 2];
    unsigned char* tv_out_flip_basic = new unsigned char[size * 2];

    Shape shape_out(1, 3, h_in, w_in);
    TensorHf4 tensor(shape_out);
    TensorHf4 tensor_basic(shape_out);
    float means[3] = {127.5f, 127.5f, 127.5f};
    float scales[3] = {1 / 127.5f, 1 / 127.5f, 1 / 127.5f};

#if COMPARE_RESULT
    LOG(INFO) << "nv21_bgr_basic compute";
    nv21_bgr_basic(nv21, 1, h_in, w_in, tmp_basic, h_in, w_in);

    LOG(INFO) << "bgr_resize_basic compute";
    bgr_resize_basic(tmp_basic, 1, h_in, w_in, bgr_basic, h_out, w_out, width_scale, height_scale);

    LOG(INFO) << "bgr_rotate_hwc_basic compute";
    if (g_angle == 90){
        bgr_rotate_hwc_basic(tmp_basic, 1, h_in, w_in, tv_out_ratote_basic, w_in, h_in, 90);
    }
    if (g_angle == 180){
        bgr_rotate_hwc_basic(tmp_basic, 1, h_in, w_in, tv_out_ratote_basic, h_in, w_in, 180);
    }
    if (g_angle == 270){
        bgr_rotate_hwc_basic(tmp_basic, 1, h_in, w_in, tv_out_ratote_basic, w_in, h_in, 270);
    }

    LOG(INFO) << "bgr_flip_hwc_basic compute";
    bgr_flip_hwc_basic(tmp_basic, 1, h_in, w_in, tv_out_flip_basic, h_in, w_in, g_flip_num);

    LOG(INFO) << "bgr_to_tensor_hwc_basic compute";
    bgr_to_tensor_hwc_basic(tmp_basic, tensor_basic, w_in, h_in, means, scales);
    // write_pgm("/data/local/tmp/lite/test_resize.pgm", h_out, w_out, tout_basic);
    //print_tensor(tout_basic);
#endif

    SaberTimer t1;

    LOG(INFO) << "saber cv resize compute";
    double to = 0;
    double min_time = 100000;
    for (int i = 0; i < test_iter; ++i) {
        t1.clear();
        t1.start();
        LOG(INFO) << "nv21_to_bgr saber compute";
        nv21_to_bgr(nv21, tmp, w_in, h_in);

        LOG(INFO) << "bgr_resize saber compute";
        bgr_resize(tmp, bgr, w_in, h_in, w_out, h_out);

        LOG(INFO) << "bgr_rotate_hwc compute";
        if (g_angle == 90){
            bgr_rotate_hwc(tmp, tv_out_ratote, w_in, h_in, 90);
        }
        if (g_angle == 180){
            bgr_rotate_hwc(tmp, tv_out_ratote, w_in, h_in, 180);
        }
        if (g_angle == 270){
            bgr_rotate_hwc(tmp, tv_out_ratote, w_in, h_in, 270);
        }

        LOG(INFO) << "bgr_flip_hwc compute";
        bgr_flip_hwc(tmp, tv_out_flip, w_in, h_in, g_flip_num);

        LOG(INFO) << "bgr_to_tensor_hwc compute";
        bgr_to_tensor_hwc(tmp, tensor, w_in, h_in, means, scales);

        t1.end();
        double tdiff = t1.get_average_ms();
        to += tdiff;
        if (tdiff < min_time) {
            min_time = tdiff;
        }
    }
    printf("saber resize total time : %.4f, avg time : %.4f\n", to, to / test_iter, min_time);
    //print_tensor(tout);
#if COMPARE_RESULT
    double max_ratio = 0;
    double max_diff = 0;
    const double eps = 1e-6f;
    LOG(INFO) << "diff, nv_to_bgr size: " << size;
    for (int i = 0; i < size * 2; i++){
        int a = tmp[i];
        int b = tmp_basic[i];
        int diff1 = a - b;
        int diff = diff1 > 0 ? diff1 : -diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
    printf("write bgr picture \n");
    write_ppm_1("/data/local/tmp/lite/test_bgr.pgm", h_in, w_in, tmp);
#endif
    delete[] tmp;
    delete[] tmp_basic;
    LOG(INFO) << "nv_to_bgr end";

#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_resie size: " << out_size;
    for (int i = 0; i < out_size; i++){
        int a = bgr[i];
        int b = bgr_basic[i];
        int diff1 = a - b;
        int diff = 0; //basic resize and saber resize 在float -> int转换时存在误差，误差范围是{-1, 1}
        if (diff1 < -1 || diff1 > 1)
            diff = diff1 < 0 ? -diff1 : diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
    printf("write bgr resize picture \n");
    write_ppm_1("/data/local/tmp/lite/test_bgr_resize.pgm", h_out, w_out, bgr);
#endif
    delete[] bgr;
    delete[] bgr_basic;
    LOG(INFO) << "bgr_resize end";
#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_flip_hwc size: " << size * 2;
    for (int i = 0; i < size * 2; i++){
        int a = tv_out_flip[i];
        int b = tv_out_flip_basic[i];
        int diff1 = a - b;
        int diff = diff1 > 0 ? diff1 : -diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
    printf("write bgr flip picture \n");
    write_ppm_1("/data/local/tmp/lite/test_bgr_flip.pgm", h_in, w_in, tv_out_flip);
#endif
    delete[] tv_out_flip;
    delete[] tv_out_flip_basic;
    LOG(INFO) << "bgr_flip_hwc end";

#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_rotate_hwc size: " << size * 2;
    for (int i = 0; i < size * 2; i++){
        int a = tv_out_ratote[i];
        int b = tv_out_ratote_basic[i];
        int diff1 = a - b;
        int diff = diff1 > 0 ? diff1 : -diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    // printf("\n");
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
    printf("write bgr rotate picture \n");
    if (g_angle == 90 || g_angle == 270)
        write_ppm_1("/data/local/tmp/lite/test_bgr_rotate.pgm", w_in, h_in, tv_out_ratote);
    else
        write_ppm_1("/data/local/tmp/lite/test_bgr_rotate.pgm", h_in, w_in, tv_out_ratote);
#endif
    delete[] tv_out_ratote;
    delete[] tv_out_ratote_basic;
    LOG(INFO) << "bgr_rotate_hwc end";

#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_to_tensor_hwc size: " << tensor.valid_size();
    float* ptr_a = tensor.data();
    float* ptr_b = tensor_basic.data();
    for (int i = 0; i < tensor.valid_size(); i++){
        int a = ptr_a[i];
        int b = ptr_b[i];
        int diff1 = a - b;
        int diff = diff1 > 0 ? diff1 : -diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
    LOG(INFO) << "bgr_to_tensor_hwc end";
#endif

}
#endif

#if 1
TEST(TestSaberLite, test_func_nv21_bgr_resize) {
    LOG(INFO) << "test_func_cv_bgr_resize start";
    // start Reshape & doInfer
    Context ctx1;
    LOG(INFO) << "set runtine context";
    PowerMode mode = g_cluster == 0? SABER_POWER_HIGH : SABER_POWER_LOW;
    ctx1.set_run_mode(mode, g_threads);
    LOG(INFO) << "test threads activated";
#pragma omp parallel
    {
#ifdef USE_OPENMP
        int thread = omp_get_num_threads();
        LOG(INFO) << "number of threads: " << thread;
#endif
    }

    int test_iter = 1;

    int w_in = g_w;
    int h_in = g_h;
    int w_out = g_ww;
    int h_out = g_hh;

    LOG(INFO) << " input tensor size, num=" << 1 << ", channel=" << \
        1 << ", height=" << h_in << ", width=" << w_in;
    LOG(INFO) << " output tensor size, num=" << 1 << ", channel=" << \
        1 << ", height=" << h_out << ", width=" << w_out;

    //Tensor<CPU, AK_UINT8> thin(shape_in);
    int size = 3 * h_in * w_in / 2;
    unsigned char* nv21 = new unsigned char[size];

    fill_tensor_host_rand(nv21, size);

    int out_size = 3 * h_out * w_out;
    unsigned char* bgr = new unsigned char[out_size];
    unsigned char* bgr_basic = new unsigned char[out_size];

    float width_scale = (float)w_in / w_out;
    float height_scale = (float)h_in / h_out;

    //bgr
    unsigned char* tmp_basic = new unsigned char[size * 2];
    unsigned char* tmp = new unsigned char[size * 2];

    unsigned char* tv_out_ratote = new unsigned char[out_size];
    unsigned char* tv_out_ratote_basic = new unsigned char[out_size];

    Shape shape_out(1, 3, h_out, w_out);
    TensorHf4 tensor(shape_out);
    TensorHf4 tensor_basic(shape_out);
    float means[3] = {127.5f, 127.5f, 127.5f};
    float scales[3] = {1 / 127.5f, 1 / 127.5f, 1 / 127.5f};

#if COMPARE_RESULT
    LOG(INFO) << "nv21_bgr_basic compute";
    nv21_bgr_basic(nv21, 1, h_in, w_in, tmp_basic, h_in, w_in);

    LOG(INFO) << "bgr_resize_basic compute";
    bgr_resize_basic(tmp_basic, 1, h_in, w_in, bgr_basic, h_out, w_out, width_scale, height_scale);

    LOG(INFO) << "nv21_bgr_basic compute";
    if (g_angle == 90){
        bgr_rotate_hwc_basic(bgr_basic, 1, h_out, w_out, tv_out_ratote_basic, w_out, h_out, 90);
    }
    if (g_angle == 180){
        bgr_rotate_hwc_basic(bgr_basic, 1, h_out, w_out, tv_out_ratote_basic, h_out, w_out, 180);
    }
    if (g_angle == 270){
        bgr_rotate_hwc_basic(bgr_basic, 1, h_out, w_out, tv_out_ratote_basic, w_out, h_out, 270);
    }

    LOG(INFO) << "nv21_bgr_basic compute";
    bgr_to_tensor_hwc_basic(tv_out_ratote_basic, tensor_basic, h_out, w_out, means, scales);
#endif

    SaberTimer t1;

    LOG(INFO) << "saber cv resize compute";
    double to = 0;
    double min_time = 100000;
    for (int i = 0; i < test_iter; ++i) {
        t1.clear();
        t1.start();
        LOG(INFO) << "nv21_to_bgr saber compute";
        nv21_to_bgr(nv21, tmp, w_in, h_in);

        LOG(INFO) << "bgr_resize saber compute";
        bgr_resize(tmp, bgr, w_in, h_in, w_out, h_out);

        LOG(INFO) << "bgr_rotate_hwc compute";
        if (g_angle == 90){
            bgr_rotate_hwc(bgr, tv_out_ratote, w_out, h_out, 90);
        }
        if (g_angle == 180){
            bgr_rotate_hwc(bgr, tv_out_ratote, w_out, h_out, 180);
        }
        if (g_angle == 270){
            bgr_rotate_hwc(bgr, tv_out_ratote, w_out, h_out, 270);
        }

        LOG(INFO) << "bgr_to_tensor_hwc compute";
        bgr_to_tensor_hwc(tv_out_ratote, tensor, h_out, w_out, means, scales);

        t1.end();
        double tdiff = t1.get_average_ms();
        to += tdiff;
        if (tdiff < min_time) {
            min_time = tdiff;
        }
    }
    printf("saber resize total time : %.4f, avg time : %.4f\n", to, to / test_iter, min_time);
    //print_tensor(tout);
#if COMPARE_RESULT
    double max_ratio = 0;
    double max_diff = 0;
    const double eps = 1e-6f;
    LOG(INFO) << "diff, nv_to_bgr size: " << size;
    for (int i = 0; i < size * 2; i++){
        int a = tmp[i];
        int b = tmp_basic[i];
        int diff1 = a - b;
        int diff = diff1 > 0 ? diff1 : -diff1;

        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }

    }
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
#endif
    delete[] tmp;
    delete[] tmp_basic;
    LOG(INFO) << "nv_to_bgr end";

#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_resie size: " << out_size;
    for (int i = 0; i < out_size; i++){
        int a = bgr[i];
        int b = bgr_basic[i];
        int diff1 = a - b;
        int diff = 0; //basic resize and saber resize 在float -> int转换时存在误差，误差范围是{-1, 1}
        if (diff1 < -1 || diff1 > 1)
            diff = diff1 < 0 ? -diff1 : diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    // printf("\n");
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
#endif
    delete[] bgr;
    delete[] bgr_basic;
    LOG(INFO) << "bgr_resize end";

#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_rotate_hwc size: " << size * 2;
    for (int i = 0; i < out_size; i++){
        int a = tv_out_ratote[i];
        int b = tv_out_ratote_basic[i];
        int diff1 = a - b;
        // int diff = diff1 > 0 ? diff1 : -diff1;
        int diff = 0;
        if (diff1 < -1 || diff1 > 1)
            diff = diff1 < 0 ? -diff1 : diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
#endif
    delete[] tv_out_ratote;
    delete[] tv_out_ratote_basic;
    LOG(INFO) << "bgr_rotate_hwc end";
#if COMPARE_RESULT
    max_ratio = 0;
    max_diff = 0;
    // const double eps = 1e-6f;
    LOG(INFO) << "diff, bgr_to_tensor_hwc size: " << tensor.valid_size();
    float* ptr_a = tensor.data();
    float* ptr_b = tensor_basic.data();
    for (int i = 0; i < tensor.valid_size(); i++){
        int a = ptr_a[i];
        int b = ptr_b[i];
        int diff1 = a - b;
        int diff = 0;
        if (diff1 < -1 || diff1 > 1)
            diff = diff1 < 0 ? -diff1 : diff1;
        if (max_diff < diff) {
            max_diff = diff;
            max_ratio = 2.0 * max_diff / (a + b + eps);
        }
    }
    // printf("\n");
    LOG(INFO) << "compare result, max diff: " << max_diff << ", max ratio: " << max_ratio;
    CHECK_EQ(fabsf(max_ratio) < 1e-5f, true) << "compute result error";
    LOG(INFO) << "bgr_to_tensor_hwc end";
#endif

}
#endif

int main(int argc, const char** argv){
    // initial logger
    //logger::init(argv[0]);
   // Env::env_init(4);
    Env::env_init();
    LOG(ERROR) << "usage: ./" << argv[0] << " img_name cluster  threads  img_h_in " << \
                " img_w_in h_out w_out flip_num angle";
    if (argc >= 2) {
        g_img_name = argv[1];
    }
    if (argc >= 3) {
        g_cluster = atoi(argv[2]);
    }

    if (argc >= 4) {
        g_threads = atoi(argv[3]);
    }

    if (argc >= 5) {
        g_h = atoi(argv[4]);//输入图像的高度，必须是灰度图像，且为pgm格式
    }
    if (argc >= 6) {
        g_w = atoi(argv[5]);//输入图像的宽度，必须是灰度图像，且为pgm格式
    }
    if (argc >= 7) {
        g_hh = atoi(argv[6]);
    }
    if (argc >= 8) {
        g_ww = atoi(argv[7]);
    }
    if (argc >= 9){
        g_flip_num = atoi(argv[8]); //翻转 x轴：1 y轴：-1 xy轴：0
    }
    if (argc >= 10){
        g_angle = atoi(argv[9]); //旋转:90 180 270
    }

    InitTest();
    RUN_ALL_TESTS(argv[0]);
    return 0;
}

